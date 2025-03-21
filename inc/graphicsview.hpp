#ifndef _GRAPHICSVIEW_HPP
#define _GRAPHICSVIEW_HPP

#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsView>
#include <QMap>
#include <QOpenGLWidget>
#include <QPixmap>
#include <qthread.h>
#include <tuple>


class SphereItem : public QGraphicsEllipseItem {
public:
	SphereItem(int radius, QColor color);
	void updateColor(QColor color);
	void setRadius(int radius);

private:
	int m_radius;
};

class ArrowItem : public QGraphicsLineItem {
public:
	ArrowItem(QLineF line);
	void updateColor(QColor startColor, QColor endColor);
	void updateArrowHead();

	void setLine(QLineF line); // override

private:
	QColor m_startColor;
	QColor m_endColor;
	QGraphicsPolygonItem* m_arrowHead;
};

class GraphicsManager : public QObject {
	Q_OBJECT
public:
	GraphicsManager(QGraphicsView* view, QObject* parent = nullptr);
	~GraphicsManager();

	void addSphereArrow(QString name, int x, int y, int x_to, int y_to, bool is_left,
						QColor color = QColor(255, 255, 204));
	void rmSphereArrow(QString name);
	void setSpherePos(QString name, int x, int y, bool is_left, bool need_flip_arrow);
	void setSphereColor(QString name, QColor color);
	void setArrowPointingTo(QString name, int x, int y);
	void setSphereRadius(QString name, int radius);
	void setArrowRot90(QString name, int num_of_90);

	int width() const;
	int height() const;

	void clear();

public Q_SLOTS:
	void setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y);
	void setDefaultSphereColorScalar(QString name, qreal scalar);

private:
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;
	QPixmap m_bgPixmap;
	QHash<QString, std::tuple<SphereItem*, ArrowItem*, QColor>> m_objects;
	int m_width;
	int m_height;
	QThread* m_thread;

	void createBackground();
	void wrap_x(int& x, bool is_left);
};

#endif // _GRAPHICSVIEW_HPP