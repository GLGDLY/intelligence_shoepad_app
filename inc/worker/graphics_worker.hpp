#ifndef _GRAPHICS_WORKER_H
#define _GRAPHICS_WORKER_H

#include <QtCore/QObject>

class GraphicsWorker : public QObject {
	Q_OBJECT
public:
	GraphicsWorker(void* mainwindow, QObject* parent = nullptr);
	~GraphicsWorker();

public Q_SLOTS:
	void updateGraphicsData();

private:
	void* main_window;
};

#endif // _GRAPHICS_WORKER_H
