#include "graphicsview.hpp"

#include <qbrush.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qpixmap.h>
#include <tuple>


Canvas::Canvas(int width, int height, QPixmap bg) : QObject(), bg_image(bg) {
	this->pixmap = new QPixmap(width, height);
	this->painter = new QPainter(pixmap);
	this->painter->setRenderHint(QPainter::Antialiasing);

	this->clear();
}

Canvas::~Canvas() {
	delete this->pixmap;
	delete this->painter;
}

void Canvas::drawCircle(int x, int y, int radius, QColor color) {
	this->painter->setPen(QPen(color, 5));
	QRadialGradient gradient(x, y, radius);
	gradient.setColorAt(0, Qt::gray);
	gradient.setColorAt(1, color);
	this->painter->setBrush(QBrush(gradient));

	this->painter->drawEllipse(x - radius, y - radius, radius * 2, radius * 2);
}

void Canvas::drawArrow(int x1, int y1, int x2, int y2) {
	this->painter->setPen(QPen(Qt::black, 5));
	QLinearGradient gradient(x1, y1, x2, y2);
	gradient.setColorAt(0, Qt::white);
	gradient.setColorAt(1, Qt::black);
	this->painter->setBrush(QBrush(gradient));

	this->painter->drawLine(x1, y1, x2, y2);
	if (x1 == x2 && y1 == y2) { // no need to draw arrow
		return;
	}
	auto angle = std::atan2(y2 - y1, x2 - x1);
	auto arrow_size = 10;
	auto arrow_x1 = x2 - arrow_size * std::cos(angle + M_PI / 6);
	auto arrow_y1 = y2 - arrow_size * std::sin(angle + M_PI / 6);
	this->painter->drawLine(x2, y2, arrow_x1, arrow_y1);
	auto arrow_x2 = x2 - arrow_size * std::cos(angle - M_PI / 6);
	auto arrow_y2 = y2 - arrow_size * std::sin(angle - M_PI / 6);
	this->painter->drawLine(x2, y2, arrow_x2, arrow_y2);
	this->painter->drawLine(arrow_x1, arrow_y1, arrow_x2, arrow_y2);
}

void Canvas::clear() {
	this->painter->eraseRect(0, 0, pixmap->width(), pixmap->height());
	this->painter->drawPixmap(0, 0, bg_image);
}

Sphere::Sphere(int x, int y, QObject* parent) : QObject(parent), sphere_x(x), sphere_y(y), sphere_radius(20) {}

Sphere::~Sphere() {}

void Sphere::setPos(int x, int y) {
	sphere_x = x;
	sphere_y = y;
}

void Sphere::setRadius(int radius) { sphere_radius = radius; }

std::tuple<int, int, int> Sphere::getSphere() const { return std::make_tuple(sphere_x, sphere_y, sphere_radius); }

Arrow::Arrow(int x1, int y1, int x2, int y2, QObject* parent)
	: QObject(parent), arrow_x1(x1), arrow_y1(y1), arrow_x2(x2), arrow_y2(y2) {}

Arrow::~Arrow() {}

void Arrow::setPos(int x1, int y1, int x2, int y2) {
	arrow_x1 = x1;
	arrow_y1 = y1;
	arrow_x2 = x2;
	arrow_y2 = y2;
}

std::tuple<int, int, int, int> Arrow::getArrow() const {
	return std::make_tuple(arrow_x1, arrow_y1, arrow_x2, arrow_y2);
}

GraphicsManager::GraphicsManager(QGraphicsView* view, QObject* parent) : QObject(parent), m_view(view) {
	auto img = QPixmap("images/insole.jpg");
	img = img.scaled(view->size(), Qt::KeepAspectRatio);

	// set image to rounded corners
	QBitmap mask(img.size());
	QPainter painter(&mask);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(img.rect(), Qt::white);
	painter.setBrush(Qt::black);
	painter.drawRoundedRect(img.rect(), 10, 10);
	painter.end();
	img.setMask(mask);

	this->m_width = img.width();
	this->m_height = img.height();

	this->m_scene = new QGraphicsScene(this);
	this->m_canvas = new Canvas(this->m_width, this->m_height, img);

	this->m_view->setScene(m_scene);
	this->reload();
}

GraphicsManager::~GraphicsManager() {
	delete m_scene;
	delete m_canvas;
	for (auto [sphere, arrow, color] : m_objects.values()) {
		delete sphere;
		delete arrow;
		delete color;
	}
}

void GraphicsManager::addSphereArrow(QString name, int x, int y, int x_to, int y_to, QColor color) {
	auto sphere = new Sphere(x, y);
	auto arrow = new Arrow(x, y, x_to, y_to);

	this->drawSphereArrow(sphere, arrow, color);

	auto obj = std::make_tuple(sphere, arrow, new QColor(color));
	m_objects.insert(name, obj);
}

void GraphicsManager::rmSphereArrow(QString name) {
	qDebug() << "Removing object";
	if (!m_objects.contains(name)) {
		return;
	}
	auto obj = m_objects[name];
	delete std::get<0>(obj);
	delete std::get<1>(obj);
	delete std::get<2>(obj);
	m_objects.remove(name);
	qDebug() << "Removed object" << m_objects.size();
	this->reload();
}

void GraphicsManager::setSpherePos(QString name, int x, int y) {
	if (!m_objects.contains(name)) {
		return;
	}
	qDebug() << "Set sphere pos";
	auto obj = m_objects[name];
	std::get<0>(obj)->setPos(x, y);
	auto arrow = std::get<1>(obj)->getArrow();
	std::get<1>(obj)->setPos(x, y, x + std::get<2>(arrow) - std::get<0>(arrow),
							 y + std::get<3>(arrow) - std::get<1>(arrow));
	this->reload();
}

void GraphicsManager::setSphereColor(QString name, QColor color) {
	if (!m_objects.contains(name)) {
		return;
	}
	auto obj = m_objects[name];
	std::get<2>(obj)->setRgb(color.rgb());
	std::get<2>(obj)->setAlpha(color.alpha());
	this->reload();
}

void GraphicsManager::setDefaultSphereColorScalar(QString name, qreal scalar) {
	if (!m_objects.contains(name)) {
		return;
	}
	auto obj = m_objects[name];
	auto color = std::get<2>(obj);

	scalar = int(fmin(fmax(scalar, 0), 1) * (255 + 204));

	const auto base = QColor(255, 255, 204); // g from 0 to 255, b from 0 to 204
	if (scalar <= 204) {
		color->setRgb(base.red(), base.green(), base.blue() - scalar);
	} else {
		color->setRgb(base.red(), base.green() - (scalar - 204), 0);
	}

	this->reload();
}

void GraphicsManager::setArrowPointingTo(QString name, int x, int y) {
	if (!m_objects.contains(name)) {
		return;
	}
	auto obj = m_objects[name];
	auto sphere_xyr = std::get<0>(obj)->getSphere();
	std::get<1>(obj)->setPos(std::get<0>(sphere_xyr), std::get<1>(sphere_xyr), x, y);
	this->reload();
}

void GraphicsManager::setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y) {
	if (!m_objects.contains(name)) {
		return;
	}
	auto obj = m_objects[name];
	auto sphere_xyr = std::get<0>(obj)->getSphere();
	auto sphere_x = std::get<0>(sphere_xyr);
	auto sphere_y = std::get<1>(sphere_xyr);
	auto arrow = std::get<1>(obj);

	auto arrow_x1 = sphere_x + fmin(fmax(sca_x, -1), 1) * 50;
	auto arrow_y1 = sphere_y + fmin(fmax(sca_y, -1), 1) * 50;
	arrow->setPos(sphere_x, sphere_y, arrow_x1, arrow_y1);
	this->reload();
}

void GraphicsManager::clear() {
	for (auto [sphere, arrow, color] : m_objects) {
		delete sphere;
		delete arrow;
		delete color;
	}
	m_objects.clear();
	this->reload();
}

void GraphicsManager::drawSphereArrow(Sphere* sphere, Arrow* arrow, QColor color) {
	auto arrow_xyxy = arrow->getArrow();
	m_canvas->drawArrow(std::get<0>(arrow_xyxy), std::get<1>(arrow_xyxy), std::get<2>(arrow_xyxy),
						std::get<3>(arrow_xyxy));

	// let sphere cover the end of the arrow
	auto sphere_xyr = sphere->getSphere();
	m_canvas->drawCircle(std::get<0>(sphere_xyr), std::get<1>(sphere_xyr), std::get<2>(sphere_xyr), color);
}

void GraphicsManager::reload() {
	this->m_canvas->clear();
	for (auto [sphere, arrow, color] : m_objects) {
		this->drawSphereArrow(sphere, arrow, *color);
	}
	this->m_scene->clear();
	this->m_scene->addPixmap(*this->m_canvas->pixmap);
}

const int GraphicsManager::width() const { return m_width; }
const int GraphicsManager::height() const { return m_height; }
