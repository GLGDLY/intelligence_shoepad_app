#ifndef _GRAPHICSVIEW_HPP
#define _GRAPHICSVIEW_HPP

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsView>
#include <QMap>
#include <QOpenGLWidget>
#include <QPixmap>
#include <QThread>
#include <qobject.h>
#include <qtmetamacros.h>
#include <tuple>

class ArrowItem : public QObject {
	Q_OBJECT
public:
	ArrowItem(QLineF line);
	~ArrowItem();
	void updateColor(QColor startColor, QColor endColor);
	void updateArrowHead();

	QGraphicsPolygonItem* arrowHead() const { return m_arrowHead; }
	QGraphicsLineItem* lineItem() const { return m_lineItem; }
	QLineF line() const { return m_lineItem->line(); }

public slots:
	void setLine(const QLineF line);

private:
	QColor m_startColor;
	QColor m_endColor;
	QGraphicsPolygonItem* m_arrowHead;
	QGraphicsLineItem* m_lineItem;
};

typedef enum {
	HEATMAP_FRAME_NEXT,
	HEATMAP_FRAME_CURRENT,
	NUM_OF_HEATMAP_FRAME,
} HeatmapFrame_t;

class HeatmapManager : public QGraphicsItem {
public:
	HeatmapManager(int width, int height, int cellSize, int radiation_decay = 0);
	~HeatmapManager();
	QRectF boundingRect() const override;
	QColor cvtColor(const int scalar) const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
	void setCellScalar(int xPos, int yPos, const int scalar);
	void setCellScalarBatch(const std::vector<std::tuple<int, int, int>>& cells);
	void clear();
	int cellSize() const;

private:
	int m_width;
	int m_height;
	int m_cellSize;
	int m_radiation_decay;
	QMap<QPair<int, int>, int> m_cells;
	int** m_cellScalars[NUM_OF_HEATMAP_FRAME];
};

class GraphicsManager : public QObject {
	Q_OBJECT
public:
	GraphicsManager(QGraphicsView* view, QObject* parent = nullptr);
	~GraphicsManager();

	void addSphereArrow(QString name, int x, int y, int x_to, int y_to, bool is_left);
	void rmSphereArrow(QString name);
	void setSpherePos(QString name, int x, int y, bool is_left, bool need_flip_arrow);
	void setArrowPointingTo(QString name, int x, int y);
	void setArrowRot90(QString name, int num_of_90);

	int width() const;
	int height() const;

	void clear();

public Q_SLOTS:
	void setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y);
	void setDefaultSphereColorScalar(QString name, qreal scalar);
	void setDefaultSphereColorScalarBatch(const std::vector<std::tuple<QString, qreal>>& data);
	void updateHeatmap(void);

private:
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;
	QPixmap m_bgPixmap;
	QHash<QString, std::tuple<ArrowItem*, int, int, int, bool>> m_objects;
	HeatmapManager* m_heatmap;
	int m_width;
	int m_height;
	QThread* m_thread;

	void createBackground();
	void wrap_x(int& x, bool is_left);
};

#endif // _GRAPHICSVIEW_HPP