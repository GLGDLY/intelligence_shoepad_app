#include "graphicsview.hpp"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <qmath.h>


SphereItem::SphereItem(int radius, QColor color)
	: QGraphicsEllipseItem(-radius, -radius, 2 * radius, 2 * radius), m_radius(radius) {
	QRadialGradient gradient(0, 0, radius);
	gradient.setColorAt(0, Qt::gray);
	gradient.setColorAt(1, color);
	setBrush(gradient);
	setPen(Qt::NoPen);
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

void SphereItem::updateColor(QColor color) {
	QRadialGradient gradient(0, 0, m_radius);
	gradient.setColorAt(0, Qt::gray);
	gradient.setColorAt(1, color);
	setBrush(gradient);
}

void SphereItem::setRadius(int radius) {
	m_radius = radius;
	setRect(-radius, -radius, 2 * radius, 2 * radius);
	updateColor(brush().color());
}

ArrowItem::ArrowItem(QLineF line) : QGraphicsLineItem(line), m_startColor(Qt::white), m_endColor(Qt::black) {
	m_arrowHead = new QGraphicsPolygonItem(this);
	updateArrowHead();
	setPen(QPen(QBrush(Qt::black), 2));
}

void ArrowItem::updateArrowHead() {
	QLineF l = line();
	qreal arrowSize = 10;
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

void ArrowItem::setLine(QLineF line) {
	QGraphicsLineItem::setLine(line);
	updateArrowHead();
}

GraphicsManager::GraphicsManager(QGraphicsView* view, QObject* parent) : QObject(parent), m_view(view) {
	m_view->setViewport(new QOpenGLWidget());
	m_view->setRenderHint(QPainter::Antialiasing);
	m_scene = new QGraphicsScene(this);
	m_scene->setBackgroundBrush(QBrush(QColor(0xf0, 0xf0, 0xf0)));
	m_view->setScene(m_scene);
	createBackground();
}

GraphicsManager::~GraphicsManager() {
	clear();
	delete m_scene;
}

void GraphicsManager::createBackground() {
	m_bgPixmap = QPixmap("images/insole.jpg");
	m_bgPixmap = m_bgPixmap.scaled(m_view->size(), Qt::KeepAspectRatio);

	QBitmap mask(m_bgPixmap.size());
	QPainter painter(&mask);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(m_bgPixmap.rect(), Qt::white);
	painter.setBrush(Qt::black);
	painter.drawRoundedRect(m_bgPixmap.rect(), 10, 10);
	m_bgPixmap.setMask(mask);

	m_width = m_bgPixmap.width();
	m_height = m_bgPixmap.height();
	m_scene->addPixmap(m_bgPixmap)->setZValue(0);
}

void GraphicsManager::addSphereArrow(QString name, int x, int y, int x_to, int y_to, QColor color) {
	SphereItem* sphere = new SphereItem(20, color);
	sphere->setPos(x, y);
	sphere->setZValue(2);

	ArrowItem* arrow = new ArrowItem(QLineF(x, y, x_to, y_to));
	arrow->setZValue(1);

	m_scene->addItem(sphere);
	m_scene->addItem(arrow);
	m_objects.insert(name, std::make_tuple(sphere, arrow, color));
}

void GraphicsManager::rmSphereArrow(QString name) {
	if (!m_objects.contains(name))
		return;
	auto obj = m_objects.take(name);
	delete std::get<0>(obj);
	delete std::get<1>(obj);
}

void GraphicsManager::setSpherePos(QString name, int x, int y) {
	if (!m_objects.contains(name))
		return;
	auto& obj = m_objects[name];
	SphereItem* sphere = std::get<0>(obj);
	ArrowItem* arrow = std::get<1>(obj);

	QPointF oldPos = sphere->pos();
	sphere->setPos(x, y);
	QLineF line = arrow->line();
	line.setP1(QPointF(x, y));
	line.setP2(line.p2() + QPointF(x - oldPos.x(), y - oldPos.y()));
	arrow->setLine(line);
}

void GraphicsManager::setSphereColor(QString name, QColor color) {
	if (!m_objects.contains(name))
		return;
	auto& obj = m_objects[name];
	std::get<0>(obj)->updateColor(color);
	std::get<2>(obj) = color;
}

void GraphicsManager::setSphereRadius(QString name, int radius) {
	if (!m_objects.contains(name))
		return;
	std::get<0>(m_objects[name])->setRadius(radius);
}

void GraphicsManager::setArrowPointingTo(QString name, int x, int y) {
	if (!m_objects.contains(name))
		return;
	auto& obj = m_objects[name];
	QPointF pos = std::get<0>(obj)->pos();
	std::get<1>(obj)->setLine(QLineF(pos.x(), pos.y(), x, y));
}

void GraphicsManager::setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y) {
	if (!m_objects.contains(name))
		return;
	auto& obj = m_objects[name];
	QPointF pos = std::get<0>(obj)->pos();
	qreal dx = qBound(-1.0, sca_x, 1.0) * 50;
	qreal dy = qBound(-1.0, sca_y, 1.0) * 50;
	std::get<1>(obj)->setLine(QLineF(pos.x(), pos.y(), pos.x() + dx, pos.y() + dy));
}

void GraphicsManager::setDefaultSphereColorScalar(QString name, qreal scalar) {
	if (!m_objects.contains(name))
		return;
	scalar = qBound(0.0, scalar, 1.0) * (255 + 204);
	QColor base(255, 255, 204);
	QColor newColor;

	if (scalar <= 204) {
		newColor = QColor(base.red(), base.green(), base.blue() - scalar);
	} else {
		newColor = QColor(base.red(), base.green() - (scalar - 204), 0);
	}
	setSphereColor(name, newColor);
}

int GraphicsManager::width() const { return m_width; }
int GraphicsManager::height() const { return m_height; }

void GraphicsManager::clear() {
	m_scene->clear();
	createBackground();
	m_objects.clear();
}