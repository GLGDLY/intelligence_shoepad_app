#include "worker/graphics_worker.hpp"

#include "mainwindow.h"

GraphicsWorker::GraphicsWorker(void* mainwindow, QObject* parent) : main_window(mainwindow), QObject(parent) {}

GraphicsWorker::~GraphicsWorker() {}

void GraphicsWorker::updateGraphicsData() {
	MainWindow* main_window = (MainWindow*)this->main_window;

	std::vector<std::tuple<QString, qreal>> heatmap_data;

	main_window->graphics_mutex.lock();
	for (auto key : main_window->graphics_data.keys()) {
		auto data = main_window->graphics_data[key];
		data = std::make_tuple(std::get<0>(data) / 600, std::get<1>(data) / 600, std::get<2>(data) / 600);
		auto num = main_window->graphics_data_num[key];
		if (num > 0) {
			main_window->graphicsManager->setArrowPointingToScalar(key, std::get<0>(data) / num,
																   std::get<1>(data) / num);
			// main_window->graphicsManager->setDefaultSphereColorScalar(key, std::get<2>(data) / num);
			heatmap_data.emplace_back(key, std::get<2>(data) / num);
		}
		main_window->graphics_data[key] = std::make_tuple(0.0, 0.0, 0.0);
		main_window->graphics_data_num[key] = 0;
	}
	main_window->graphics_mutex.unlock();
	main_window->graphicsManager->setDefaultSphereColorScalarBatch(heatmap_data);
}