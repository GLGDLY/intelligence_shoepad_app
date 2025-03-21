#include "worker/graphics_worker.hpp"

#include "mainwindow.h"

GraphicsWorker::GraphicsWorker(QObject* parent) : QObject(parent) {}

GraphicsWorker::~GraphicsWorker() {}

void GraphicsWorker::updateGraphicsData() {
	MainWindow* main_window = (MainWindow*)this->parent();
	main_window->graphics_mutex.lock();
	for (auto key : main_window->graphics_data.keys()) {
		auto data = main_window->graphics_data[key];
		auto num = main_window->graphics_data_num[key];
		if (num > 0) {
			main_window->graphicsManager->setArrowPointingToScalar(key, std::get<0>(data) / num,
																   std::get<1>(data) / num);
			main_window->graphicsManager->setDefaultSphereColorScalar(key, std::get<2>(data) / num);
		}
		main_window->graphics_data[key] = std::make_tuple(0.0, 0.0, 0.0);
		main_window->graphics_data_num[key] = 0;
	}
	main_window->graphics_mutex.unlock();
}