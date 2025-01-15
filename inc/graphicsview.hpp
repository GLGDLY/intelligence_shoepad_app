#ifndef _GRAPHICSVIEW_HPP
#define _GRAPHICSVIEW_HPP

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QOpenGLWidget>
#include <qmap.h>
#include <qpixmap.h>


// Base canvas that can be used to draw on
class Canvas : public QOpenGLWidget {
	Q_OBJECT
public:
	Canvas(int width, int height, QPixmap bg);
	~Canvas();

	void drawCircle(int x, int y, int radius, QColor color = Qt::black);
	void drawArrow(int x1, int y1, int x2, int y2);

	void clear();

private:
	QPixmap *pixmap, bg_image;
	QPainter* painter;

	friend class GraphicsManager;
};

class Sphere : public QObject {
	Q_OBJECT
public:
	Sphere(int x, int y, QObject* parent = nullptr);
	~Sphere();

	void setPos(int x, int y);
	void setRadius(int radius);

	std::tuple<int, int, int> getSphere() const;

private:
	int sphere_x, sphere_y;
	int sphere_radius;
};

class Arrow : public QObject {
	Q_OBJECT
public:
	Arrow(int x1, int y1, int x2, int y2, QObject* parent = nullptr);
	~Arrow();

	void setPos(int x1, int y1, int x2, int y2);

	std::tuple<int, int, int, int> getArrow() const;

private:
	int arrow_x1, arrow_y1, arrow_x2, arrow_y2;
};

class GraphicsManager : public QObject {
	Q_OBJECT
public:
	GraphicsManager(QGraphicsView* view, QObject* parent = nullptr);
	~GraphicsManager();

	void addSphereArrow(QString name, int x, int y, int x_to, int y_to, QColor color = QColor(255, 255, 204));
	void rmSphereArrow(QString name);

	void setSpherePos(QString name, int x, int y);

	void setSphereColor(QString name, QColor color);
	void setDefaultSphereColorScalar(QString name, qreal scalar);

	void setArrowPointingTo(QString name, int x, int y);
	void setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y);

	const int width() const;
	const int height() const;

	void clear();

private:
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;
	Canvas* m_canvas;
	QMap<QString, std::tuple<Sphere*, Arrow*, QColor*>> m_objects;

	int m_width, m_height;

	void drawSphereArrow(Sphere* sphere, Arrow* arrow, QColor color);
	void reload();
};


#endif // _GRAPHICSVIEW_HPP
