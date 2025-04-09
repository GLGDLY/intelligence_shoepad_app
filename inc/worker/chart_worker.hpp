#ifndef _CHART_WORKER_H
#define _CHART_WORKER_H

#include <QtCore/QObject>

class ChartWorker : public QObject {
	Q_OBJECT
public:
	ChartWorker(void* mainwindow, QObject* parent = nullptr);
	~ChartWorker();

public Q_SLOTS:
	void updateChartData();

private:
	void* main_window;
};

#endif // _CHART_WORKER_H
