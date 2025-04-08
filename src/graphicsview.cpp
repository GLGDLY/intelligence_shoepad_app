#include "graphicsview.hpp"

#include <QGraphicsItem>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <qobjectdefs.h>


ArrowItem::ArrowItem(QLineF line) : m_startColor(Qt::white), m_endColor(Qt::black) {
	m_lineItem = new QGraphicsLineItem(line);
	m_arrowHead = new QGraphicsPolygonItem();
	updateArrowHead();
	m_lineItem->setPen(QPen(QBrush(Qt::black), 2));
}

ArrowItem::~ArrowItem() {
	delete m_lineItem;
	delete m_arrowHead;
}

void ArrowItem::updateArrowHead() {
	QLineF l = line();
	static const qreal arrowSize = 10;
	double angle = std::atan2(l.dy(), l.dx());

	QPointF arrowP1 = l.p2() - QPointF(cos(angle + M_PI / 6) * arrowSize, sin(angle + M_PI / 6) * arrowSize);
	QPointF arrowP2 = l.p2() - QPointF(cos(angle - M_PI / 6) * arrowSize, sin(angle - M_PI / 6) * arrowSize);

	QPolygonF arrowHead;
	arrowHead << l.p2() << arrowP1 << arrowP2;
	m_arrowHead->setPolygon(arrowHead);

	QLinearGradient gradient(l.p1(), l.p2());
	gradient.setColorAt(0, m_startColor);
	gradient.setColorAt(1, m_endColor);
	m_arrowHead->setBrush(gradient);
}

void ArrowItem::updateColor(QColor startColor, QColor endColor) {
	m_startColor = startColor;
	m_endColor = endColor;
	updateArrowHead();
}

void ArrowItem::setLine(const QLineF line) {
	m_lineItem->setLine(line);
	updateArrowHead();
}

HeatmapManager::HeatmapManager(int width, int height, int cellSize, int radiation_decay)
	: m_width(width), m_height(height), m_cellSize(cellSize), m_radiation_decay(radiation_decay) {
	const int cellWidth = width / cellSize;
	const int cellHeight = height / cellSize;
	for (int idx = 0; idx < 2; idx++) {
		m_cellScalars[idx] = new int*[cellWidth];
		for (int i = 0; i < cellWidth; ++i) {
			m_cellScalars[idx][i] = new int[cellHeight]();
		}
	}
}

HeatmapManager::~HeatmapManager() {
	for (int idx = 0; idx < 2; idx++) {
		for (int i = 0; i < m_width / m_cellSize; ++i) {
			delete[] m_cellScalars[idx][i];
		}
		delete[] m_cellScalars[idx];
	}
}

QRectF HeatmapManager::boundingRect() const { return QRectF(0, 0, m_width, m_height); }

QColor HeatmapManager::cvtColor(const int scalar) const {
	if (scalar <= 204) {
		return QColor(255, 255, 204 - scalar);
	} else {
		return QColor(255, fmax(0, 255 - (scalar - 204)), 0);
	}
}

void HeatmapManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	static bool do_once = true; // ensure update on startup
	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::NoBrush);
	painter->setOpacity(0.5);
	for (int i = 0; i < m_width / m_cellSize; ++i) {
		for (int j = 0; j < m_height / m_cellSize; ++j) {
			if (m_cellScalars[HEATMAP_FRAME_NEXT][i][j] == m_cellScalars[HEATMAP_FRAME_CURRENT][i][j] && !do_once) {
				continue;
			}
			QColor color = cvtColor(m_cellScalars[HEATMAP_FRAME_NEXT][i][j]);
			painter->setBrush(color);
			painter->drawRect(i * m_cellSize, j * m_cellSize, m_cellSize, m_cellSize);
			m_cellScalars[HEATMAP_FRAME_CURRENT][i][j] = m_cellScalars[HEATMAP_FRAME_NEXT][i][j];
			// m_cellScalars[HEATMAP_FRAME_NEXT][i][j] = 0;
		}
	}
	do_once = false;
}

void HeatmapManager::setCellScalar(int xPos, int yPos, const int scalar) {
	int cellX = xPos / m_cellSize;
	int cellY = yPos / m_cellSize;
	if (cellX < 0 || cellX >= m_width / m_cellSize || cellY < 0 || cellY >= m_height / m_cellSize) {
		return;
	}
	QPair<int, int> cell(cellX, cellY);

	// add new color
	if (scalar > 0) {
		m_cells[cell] = scalar;
		m_cellScalars[HEATMAP_FRAME_NEXT][cellX][cellY] = scalar;
		const int radiation_cell_num = scalar / m_radiation_decay;
		for (int i = -radiation_cell_num; i <= radiation_cell_num; ++i) {
			for (int j = -radiation_cell_num; j <= radiation_cell_num; ++j) {
				if (i * i + j * j <= radiation_cell_num * radiation_cell_num) {
					int newCellX = cellX + i;
					int newCellY = cellY + j;
					if (newCellX >= 0 && newCellX < m_width / m_cellSize && newCellY >= 0
						&& newCellY < m_height / m_cellSize) {
						m_cellScalars[HEATMAP_FRAME_NEXT][newCellX][newCellY] +=
							scalar * (1 - qSqrt(i * i + j * j) / radiation_cell_num);
					}
				}
			}
		}
	} else {
		m_cells.remove(cell);
	}

	// update();
}

void HeatmapManager::setCellScalarBatch(const std::vector<std::tuple<int, int, int>>& cells) {
	for (int i = 0; i < m_width / m_cellSize; ++i) {
		for (int j = 0; j < m_height / m_cellSize; ++j) {
			m_cellScalars[HEATMAP_FRAME_NEXT][i][j] = 0;
		}
	}

	for (const auto& cell : cells) {
		int x = std::get<0>(cell);
		int y = std::get<1>(cell);
		int scalar = std::get<2>(cell);
		setCellScalar(x, y, scalar);
	}
	update();
}

void HeatmapManager::clear() {
	m_cells.clear();
	update();
}

int HeatmapManager::cellSize() const { return m_cellSize; }

GraphicsManager::GraphicsManager(QGraphicsView* view, QObject* parent)
	: QObject(parent), m_view(view), m_thread(new QThread), m_heatmap(nullptr) {
	m_thread->setObjectName("GraphicsManagerThread");
	m_thread->start();
	moveToThread(m_thread);
	m_view->setViewport(new QOpenGLWidget());
	m_view->setRenderHint(QPainter::Antialiasing, false);
	m_scene = new QGraphicsScene();
	m_scene->setBackgroundBrush(QBrush(QColor(0xf0, 0xf0, 0xf0)));
	m_view->setScene(m_scene);
	createBackground();
}

GraphicsManager::~GraphicsManager() {
	clear();
	delete m_heatmap;
	delete m_scene;
}

void GraphicsManager::createBackground() {
	m_bgPixmap = QPixmap("images/insole.jpg");
	m_bgPixmap = m_bgPixmap.scaled(m_view->size(), Qt::KeepAspectRatio);

	QBitmap mask(m_bgPixmap.size());
	QPainter painter(&mask);
	painter.fillRect(m_bgPixmap.rect(), Qt::white);
	painter.setBrush(Qt::black);
	painter.drawRoundedRect(m_bgPixmap.rect(), 10, 10);
	m_bgPixmap.setMask(mask);

	QPen pen(Qt::black, 4);
	QLineF line(m_bgPixmap.width() / 2, 0, m_bgPixmap.width() / 2, m_bgPixmap.height());
	m_scene->addLine(line, pen)->setZValue(1);

	m_width = m_bgPixmap.width();
	m_height = m_bgPixmap.height();
	m_scene->addPixmap(m_bgPixmap)->setZValue(0);

	if (!m_heatmap) {
		m_heatmap = new HeatmapManager(m_width, m_height, 5, 50);
		m_heatmap->setZValue(1);
		m_scene->addItem(m_heatmap);
	} else {
		m_heatmap->clear();
		m_heatmap->update();
	}
}

void GraphicsManager::wrap_x(int& x, bool is_left) {
	x = qBound(0, x, m_width / 2);
	if (!is_left)
		x = m_width - x;
}

void GraphicsManager::addSphereArrow(QString name, int x, int y, int x_to, int y_to, bool is_left) {
	wrap_x(x, is_left);
	wrap_x(x_to, is_left);

	m_heatmap->setCellScalar(x, y, 1);
	ArrowItem* arrow = new ArrowItem(QLineF(x, y, x_to, y_to));
	arrow->arrowHead()->setZValue(2);
	arrow->lineItem()->setZValue(2);
	m_scene->addItem(arrow->arrowHead());
	m_scene->addItem(arrow->lineItem());
	m_objects.insert(name, std::make_tuple(arrow, 1, x, y, is_left));
}

void GraphicsManager::rmSphereArrow(QString name) {
	if (!m_objects.contains(name))
		return;

	auto obj = m_objects.take(name);
	ArrowItem* arrow = std::get<0>(obj);
	int x = std::get<2>(obj);
	int y = std::get<3>(obj);

	m_heatmap->setCellScalar(x, y, 0);
	m_scene->removeItem(arrow->arrowHead());
	m_scene->removeItem(arrow->lineItem());
	delete arrow;
}

void GraphicsManager::setSpherePos(QString name, int x, int y, bool is_left, bool need_flip_arrow) {
	if (!m_objects.contains(name))
		return;

	wrap_x(x, is_left);

	auto& obj = m_objects[name];
	ArrowItem* arrow = std::get<0>(obj);
	int scalar = std::get<1>(obj);
	int oldX = std::get<2>(obj);
	int oldY = std::get<3>(obj);
	bool oldIsLeft = std::get<4>(obj);

	m_heatmap->setCellScalar(oldX, oldY, 0);

	int cellSize = m_heatmap->cellSize();
	int newCellX = x / cellSize;
	int newCellY = y / cellSize;
	int newX = newCellX * cellSize + cellSize / 2;
	int newY = newCellY * cellSize + cellSize / 2;

	m_heatmap->setCellScalar(newX, newY, scalar);
	std::get<2>(obj) = x;
	std::get<3>(obj) = y;
	std::get<4>(obj) = is_left;

	QLineF line = arrow->line();
	qreal dx = line.dx();
	qreal dy = line.dy();

	if (need_flip_arrow)
		line = QLineF(newX, newY, newX - dx, newY - dy);
	else
		line = QLineF(newX, newY, newX + dx, newY + dy);

	arrow->setLine(line);
}

void GraphicsManager::setArrowPointingTo(QString name, int x, int y) {
	if (!m_objects.contains(name))
		return;

	auto& obj = m_objects[name];
	ArrowItem* arrow = std::get<0>(obj);
	int sphereX = std::get<2>(obj);
	int sphereY = std::get<3>(obj);
	arrow->setLine(QLineF(sphereX, sphereY, x, y));
}

void GraphicsManager::setArrowRot90(QString name, int num_of_90) {
	if (!m_objects.contains(name))
		return;

	auto& obj = m_objects[name];
	QLineF line = std::get<0>(obj)->line();
	line.setAngle(line.angle() - num_of_90 * 90);
	std::get<0>(obj)->setLine(line);
}

void GraphicsManager::setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y) {
	if (!m_objects.contains(name))
		return;

	auto& obj = m_objects[name];
	ArrowItem* arrow = std::get<0>(obj);
	int x = std::get<2>(obj);
	int y = std::get<3>(obj);
	qreal dx = qBound(-1.0, sca_x, 1.0) * 50;
	qreal dy = qBound(-1.0, sca_y, 1.0) * 50;
	// arrow->setLine(QLineF(x, y, x + dx, y + dy)); // need to switch back to correct thread
	QMetaObject::invokeMethod(arrow, "setLine", Qt::QueuedConnection, Q_ARG(QLineF, QLineF(x, y, x + dx, y + dy)));
}

void GraphicsManager::setDefaultSphereColorScalar(QString name, qreal scalar) {
	if (!m_objects.contains(name))
		return;

	scalar = qBound(0.0, scalar, 1.0) * 459;
	auto& obj = m_objects[name];
	std::get<1>(obj) = scalar;
	int x = std::get<2>(obj);
	int y = std::get<3>(obj);
	this->m_heatmap->setCellScalar(x, y, scalar);
}

void GraphicsManager::setDefaultSphereColorScalarBatch(const std::vector<std::tuple<QString, qreal>>& data) {
	std::vector<std::tuple<int, int, int>> cells;
	for (const auto& item : data) {
		QString name = std::get<0>(item);
		qreal scalar = std::get<1>(item);
		if (!m_objects.contains(name))
			continue;

		scalar = qBound(0.0, scalar, 1.0) * 459;
		auto& obj = m_objects[name];
		std::get<1>(obj) = scalar;
		int x = std::get<2>(obj);
		int y = std::get<3>(obj);
		cells.emplace_back(x, y, scalar);
	}
	if (!cells.empty()) {
		this->m_heatmap->setCellScalarBatch(cells);
	}
}

void GraphicsManager::updateHeatmap() { this->m_heatmap->update(); }

int GraphicsManager::width() const { return m_width; }
int GraphicsManager::height() const { return m_height; }

void GraphicsManager::clear() {
	m_scene->clear();
	m_objects.clear();
	createBackground();
}