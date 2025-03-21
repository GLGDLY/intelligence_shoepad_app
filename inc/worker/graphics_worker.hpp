#ifndef _GRAPHICS_WORKER_H
#define _GRAPHICS_WORKER_H

#include <QtCore/QObject>

class GraphicsWorker : public QObject {
	Q_OBJECT
public:
	GraphicsWorker(QObject* parent);
	~GraphicsWorker();

public Q_SLOTS:
	void updateGraphicsData();
};

#endif // _GRAPHICS_WORKER_H
