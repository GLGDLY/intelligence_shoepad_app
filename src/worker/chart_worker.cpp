#include "worker/chart_worker.hpp"

#include "mainwindow.h"

ChartWorker::ChartWorker(QObject* parent) : QObject(parent) {}

ChartWorker::~ChartWorker() {}

void ChartWorker::updateChartData() {
	MainWindow* main_window = (MainWindow*)this->parent();
	main_window->data_queue_mutex.lock();
	if (!main_window->is_data_queue_updated) {
		main_window->data_queue_mutex.unlock();
		return;
	}
	main_window->is_data_queue_updated = false;

	for (const DataPoint& data : main_window->data_queue) {
		main_window->chart_data[0].append((QPointF){data.time_sec, data.X});
		main_window->chart_data[1].append((QPointF){data.time_sec, data.Y});
		main_window->chart_data[2].append((QPointF){data.time_sec, data.Z});
		qreal d[3] = {data.X, data.Y, data.Z};
		for (int i = 0; i < 3; i++) {
			if (main_window->chart_data[i].size() > main_window->data_series_size) {
				main_window->chart_data[i].removeFirst();
			}

			const qreal minY = qMin(qMin(std::get<0>(main_window->chart_range_y[i]), d[i]),
									std::get<0>(main_window->chart_range_y[i]));
			const qreal maxY = qMax(qMax(std::get<1>(main_window->chart_range_y[i]), d[i]),
									std::get<1>(main_window->chart_range_y[i]));
			main_window->chart_range_y[i] = std::make_tuple(minY, maxY);
		}
	}
	main_window->data_queue.clear();
	main_window->data_queue_mutex.unlock();

	if (main_window->chart_data[0].size() == 0) {
		return;
	}

	main_window->chartView[0]->setUpdatesEnabled(false);
	main_window->chartView[1]->setUpdatesEnabled(false);
	main_window->chartView[2]->setUpdatesEnabled(false);

	main_window->series[0]->clear();
	main_window->series[1]->clear();
	main_window->series[2]->clear();
	main_window->series[0]->replace(main_window->chart_data[0]);
	main_window->series[1]->replace(main_window->chart_data[1]);
	main_window->series[2]->replace(main_window->chart_data[2]);

	main_window->chartView[0]->setUpdatesEnabled(true);
	main_window->chartView[1]->setUpdatesEnabled(true);
	main_window->chartView[2]->setUpdatesEnabled(true);

	for (int i = 0; i < 3; i++) {
		qreal minX = main_window->series[i]->points().first().x();
		qreal maxX = main_window->series[i]->points().last().x();
		const qreal padding =
			ceil((std::get<1>(main_window->chart_range_y[i]) - std::get<0>(main_window->chart_range_y[i])) * 0.2);
		main_window->chart[i]->axes(Qt::Horizontal).back()->setRange(minX, maxX);
		main_window->chart[i]
			->axes(Qt::Vertical)
			.back()
			->setRange(std::get<0>(main_window->chart_range_y[i]) - padding,
					   std::get<1>(main_window->chart_range_y[i]) + padding);

		main_window->chart[i]->update();
		main_window->chartView[i]->update();
	}
}