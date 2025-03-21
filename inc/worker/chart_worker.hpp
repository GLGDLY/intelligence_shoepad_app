#ifndef _CHART_WORKER_H
#define _CHART_WORKER_H

#include <QtCore/QObject>

class ChartWorker : public QObject {
	Q_OBJECT
public:
	ChartWorker(QObject* parent);
	~ChartWorker();

public Q_SLOTS:
	void updateChartData();
};

#endif // _CHART_WORKER_H
