#include "mainwindow.h"

#include "./ui_mainwindow.h"
#include "data_recorder.hpp"
#include "infobox.hpp"

#include <QFileDialog>
#include <QGraphicsEffect>
#include <QLineEdit>
#include <QPushButton>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QValueAxis>
#include <QtLogging>
#include <QtMinMax>
#include <qdebug.h>


const QString esp_status_label_style[] = {"color: #ff0000; background-color: #cfcfcf; border-radius: 5px;",
										  "color: #00ff00; background-color: #cfcfcf; border-radius: 5px;"};

static qint64 secToNanoSec(qint64 sec) { return sec * 1000 * 1000 * 1000; }
static double NanoSecToSec(qint64 ns) { return ns / 1000.0 / 1000.0 / 1000.0; }

static qint64 secToMSec(qint64 sec) { return sec * 1000; }
static double MSecToSec(qint64 ms) { return ms / 1000.0; }

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, comboBox(new QComboBox())
	, esp_status_label(new QLabel())
	, chartView{new QChartView(), new QChartView(), new QChartView()}
	, chart{new QChart(), new QChart(), new QChart()}
	, series{new QLineSeries(), new QLineSeries(), new QLineSeries()}
	, chart_update_timer(new QTimer())
	, chart_update_thread(new QThread())
	, graphics_update_timer(new QTimer())
	, graphics_update_thread(new QThread())
	, mqtt_state_btn(new QPushButton())
	, start_stop_btn(new QPushButton())
	, mqtt_state(QMqttClient::ClientState::Disconnected)
	, mqtt_last_received_timer(new QTimer())
	, mqtt(new MqttApp())
	, x_input(new QLineEdit())
	, y_input(new QLineEdit())
	, xy_save_button(new QPushButton("Save"))
	, sensor_recalibration_button(new QPushButton("Recalibrate"))
	, rot_button(new QPushButton("Rotate"))
	, xy_left_btn(new QRadioButton("Left"))
	, xy_right_btn(new QRadioButton("Right"))
	, data_clear_flags()
	, settings(new Settings())
	, chart_worker(new ChartWorker(this))
	, graphics_worker(new GraphicsWorker(this)) {
	ui->setupUi(this);

	this->setWindowTitle(tr("Intelligence Shoepad"));
	this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);
	this->setFixedSize(this->size());
	this->setStyleSheet("QMainWindow { background-color: #f0f0f0; }");

	// graphicsView
	this->ui->graphicsView->setStyleSheet("QGraphicsView { border: 0px solid #000; }");
	this->graphicsManager = new GraphicsManager(this->ui->graphicsView);

	// comboBox
	comboBox->setInsertPolicy(QComboBox::InsertAtBottom);
	comboBox->setStyle(QStyleFactory::create("Fusion"));
	connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateChartSelect);
	// add to mainwindow on (450, 0)
	comboBox->setGeometry(450 + 10, 10, this->width() - 450 - 20 - 100, 25);
	this->layout()->addWidget(comboBox);

	// esp_status_label
	esp_status_label->setGeometry(comboBox->x() + comboBox->width() + 5, 10, 100, 25);
	esp_status_label->setStyleSheet(esp_status_label_style[0]);
	esp_status_label->setFont(QFont("Arial", 10, QFont::Bold));
	esp_status_label->setAlignment(Qt::AlignCenter);
	esp_status_label->setText("N.A.");
	this->layout()->addWidget(esp_status_label);

	// elapsed_timer
	this->elapsed_timer.start();
	this->start_time = this->getNowMicroSec();

	// chart
	auto chart_height = (this->height() - comboBox->height() - comboBox->y()) / 3;
	for (int i = 0; i < 3; i++) {
		chart[i]->setTheme(QChart::ChartThemeHighContrast);
		// chart[i]->setAnimationOptions(QChart::AllAnimations);
		// chart[i]->setAnimationDuration(50);
		chart[i]->setAnimationOptions(QChart::NoAnimation);
		chart[i]->legend()->hide();
		chart[i]->setLocalizeNumbers(true);
		chart[i]->setLocale(QLocale(QLocale::English));

		/* !===== for testing =====! */
		series[i]->append(series[i]->points().size(), i + 1);
		series[i]->append(series[i]->points().size(), i + 10);
		series[i]->append(series[i]->points().size(), i + 3);
		series[i]->append(series[i]->points().size(), i - 2);
		series[i]->append(series[i]->points().size(), i + 5);
		/* !======================! */

		series[i]->setUseOpenGL(true);
		chart[i]->addSeries(series[i]);
		chart[i]->createDefaultAxes();

		QValueAxis* axisX = (QValueAxis*)chart[i]->axes(Qt::Horizontal).back();
		axisX->setTitleText("Time");
		axisX->setTitleFont(QFont("Arial", 10));
		axisX->setMinorGridLineVisible(false);
		axisX->setGridLineVisible(false);

		QValueAxis* axisY = (QValueAxis*)chart[i]->axes(Qt::Vertical).back();
		axisY->setTitleFont(QFont("Arial", 10));
		axisY->setMinorGridLineVisible(false);
		axisY->setGridLineVisible(false);

		chart[i]->addAxis(axisX, Qt::AlignBottom);
		chart[i]->addAxis(axisY, Qt::AlignLeft);

		switch (i) {
			case 0:
				series[i]->setColor(Qt::red);
				axisY->setTitleText("Value X");
				break;
			case 1:
				series[i]->setColor(Qt::green);
				axisY->setTitleText("Value Y");
				break;
			case 2:
				series[i]->setColor(Qt::blue);
				axisY->setTitleText("Value Z");
				break;
			default: break;
		}

		// chartView[i]->setRenderHint(QPainter::Antialiasing);
		chartView[i]->setRenderHint(QPainter::Antialiasing, false);
		chartView[i]->setGeometry(450, comboBox->height() + comboBox->y() + i * chart_height, this->width() - 450,
								  chart_height);
		chartView[i]->setChart(chart[i]);
		this->layout()->addWidget(chartView[i]);
	}
	this->reloadChart();

	// connect(this->chart_update_timer, &QTimer::timeout, this, &MainWindow::updateChartData);
	connect(this->chart_update_timer, &QTimer::timeout, this->chart_worker, &ChartWorker::updateChartData,
			Qt::QueuedConnection);
	this->chart_update_timer->setInterval(100); // 10fps update interval
	this->chart_worker->moveToThread(this->chart_update_thread);
	this->chart_update_timer->moveToThread(this->chart_update_thread);
	this->chart_update_thread->start();
	QMetaObject::invokeMethod(this->chart_update_timer, "start", Qt::QueuedConnection);

	// connect(this->graphics_update_timer, &QTimer::timeout, this, &MainWindow::updateGraphicsData);
	connect(this->graphics_update_timer, &QTimer::timeout, this->graphics_worker, &GraphicsWorker::updateGraphicsData,
			Qt::QueuedConnection);
	this->graphics_update_timer->setInterval(20); // 50fps update interval
	this->graphics_worker->moveToThread(this->graphics_update_thread);
	this->graphics_update_timer->moveToThread(this->graphics_update_thread);
	this->graphics_update_thread->start();
	QMetaObject::invokeMethod(this->graphics_update_timer, "start", Qt::QueuedConnection);

	// xy input box
	QString x_placeholder = "X: 0-%1";
	QString y_placeholder = "Y: 0-%1";
	x_input->setPlaceholderText(x_placeholder.arg(this->graphicsManager->width() / 2));
	x_input->setGeometry(10, 10, 60, 25);
	x_input->setStyleSheet("QLineEdit { background-color:rgb(202, 202, 202); color: #000; }");
	x_input->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(x_input);

	y_input->setPlaceholderText(y_placeholder.arg(this->graphicsManager->height()));
	y_input->setGeometry(75, 10, 60, 25);
	y_input->setStyleSheet("QLineEdit { background-color: rgb(202, 202, 202); color: #000; }");
	y_input->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(y_input);

	// xy save button
	xy_save_button->setGeometry(140, 10, 50, 25);
	xy_save_button->setStyleSheet("QPushButton { background-color: #a9a9a9; color: #000; }");
	xy_save_button->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(xy_save_button);
	connect(xy_save_button, &QPushButton::clicked, this, &MainWindow::xySaveButtonClicked);

	// sensor recalibration button
	sensor_recalibration_button->setGeometry(195, 10, 100, 25);
	sensor_recalibration_button->setStyleSheet("QPushButton { background-color: #a9a9a9; color: #000; }");
	sensor_recalibration_button->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(sensor_recalibration_button);
	connect(sensor_recalibration_button, &QPushButton::clicked, this, &MainWindow::sensorRecalibrationButtonClicked);

	// Rotate button
	rot_button->setGeometry(300, 10, 50, 25);
	rot_button->setStyleSheet("QPushButton { background-color: #a9a9a9; color: #000; }");
	rot_button->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(rot_button);
	connect(rot_button, &QPushButton::clicked, this, &MainWindow::rotButtonClicked);

	// xy left or right btn
	xy_left_btn->setGeometry(355, 5, 50, 20);
	xy_left_btn->setStyleSheet("QRadioButton { color: #000; }");
	xy_left_btn->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(xy_left_btn);
	xy_right_btn->setGeometry(355, 25, 50, 20);
	xy_right_btn->setStyleSheet("QRadioButton { color: #000; }");
	xy_right_btn->setStyle(QStyleFactory::create("Fusion"));
	this->layout()->addWidget(xy_right_btn);

	connect(xy_left_btn, &QRadioButton::clicked, this, &MainWindow::xySaveButtonClicked);
	connect(xy_right_btn, &QRadioButton::clicked, this, &MainWindow::xySaveButtonClicked);

	// mqtt status bar
	QGraphicsDropShadowEffect* effect0 = new QGraphicsDropShadowEffect(mqtt_state_btn);
	effect0->setBlurRadius(5);
	effect0->setOffset(2, 2);
	effect0->setColor(QColor(0, 0, 0, 255 * 0.4));
	mqtt_state_btn->setGraphicsEffect(effect0);
	mqtt_state_btn->setText("MQTT: Disconnected");
	mqtt_state_btn->setFont(QFont("Calibri", 11, QFont::Medium));
	mqtt_state_btn->setStyleSheet("QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
	mqtt_state_btn->setToolTip("Click to select JSON file to replay");
	mqtt_state_btn->setGeometry(10, this->height() - 25 - 10, 220, 30);
	connect(mqtt_state_btn, &QPushButton::clicked, this, &MainWindow::mqttStateBtnClicked);
	this->layout()->addWidget(mqtt_state_btn);

	// start_stop_btn
	QGraphicsDropShadowEffect* effect1 = new QGraphicsDropShadowEffect(start_stop_btn);
	effect1->setBlurRadius(5);
	effect1->setOffset(2, 2);
	effect1->setColor(QColor(0, 0, 0, 255 * 0.4));
	start_stop_btn->setGraphicsEffect(effect1);
	start_stop_btn->setText("Start record");
	start_stop_btn->setFont(QFont("Calibri", 11, QFont::Medium));
	start_stop_btn->setStyleSheet("QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");

	start_stop_btn->setToolTip("Click to start/stop recording or replaying");
	start_stop_btn->setGeometry(240, this->height() - 25 - 10, 100, 30);
	connect(start_stop_btn, &QPushButton::clicked, this, &MainWindow::startStopBtnClicked);
	this->layout()->addWidget(start_stop_btn);

	// mqtt
	mqtt->connect_client_signal(&QMqttClient::stateChanged, this, &MainWindow::updateMQTTStatus);
	connect(mqtt, &MqttApp::dataReceived, this, &MainWindow::updateData);
	connect(mqtt, &MqttApp::calEndReceived, this, &MainWindow::updateCalEndStatus);
	connect(mqtt, &MqttApp::updateEspStatus, this, &MainWindow::updateEspStatus);

	// mqtt timer
	connect(mqtt_last_received_timer, &QTimer::timeout, this, &MainWindow::updateMQTTLastReceived);
	mqtt_last_received_timer->start(1000);

	// recorder
	connect(&recorder, &DataRecorder::playbackData, this, &MainWindow::processData);
	connect(&recorder, &DataRecorder::replayStarted, this, &MainWindow::clear);
	connect(&recorder, &DataRecorder::replayFinished, this, &MainWindow::replayFinished);
}

MainWindow::~MainWindow() {
	// Stop and clean up threads
	if (chart_update_thread->isRunning()) {
		chart_update_timer->stop();
		chart_update_thread->quit();
		chart_update_thread->wait();
	}
	if (graphics_update_thread->isRunning()) {
		graphics_update_timer->stop();
		graphics_update_thread->quit();
		graphics_update_thread->wait();
	}

	delete ui;
	for (int i = 0; i < 3; i++) {
		delete chartView[i];
		delete chart[i];
		delete series[i];
	}
	delete comboBox;
	delete esp_status_label;
	delete mqtt_state_btn;
	delete mqtt;
	for (auto value : this->data_map.values()) {
		delete value;
	}
	if (this->graphicsManager) {
		delete this->graphicsManager;
	}
}

const qint64 MainWindow::getNowNanoSec() const { return this->elapsed_timer.nsecsElapsed(); }
const qint64 MainWindow::getNowMicroSec() const { return this->elapsed_timer.nsecsElapsed() / 1000; }

void MainWindow::updateMQTTLastReceived() {
	/* Testing */
	// static bool do_once = true;
	// if (do_once) {
	// 	this->updateData(QByteArray("0/100,200,300,400"), QMqttTopicName("esp/test_esp/d/0"));
	// 	this->updateData(QByteArray("500/200,300,400,500"), QMqttTopicName("esp/test_esp/d/0"));
	// 	// this->updateCalEndStatus("test_esp", "0");
	// 	this->updateData(QByteArray("1000/100,900,800,700"), QMqttTopicName("esp/test_esp/d/0"));
	// 	this->updateData(QByteArray("2000/400,500,600,700"), QMqttTopicName("esp/test_esp/d/0"));
	// 	do_once = false;
	// }
	/* Testing */

	QStringList split = this->comboBox->currentText().split("_");
	QString key;
	for (int i = 0; i < split.size() - 1; i++) {
		key += split.at(i);
		if (i != split.size() - 2) {
			key += "_";
		}
	}
	if (!this->esp_status_map.contains(key)) {
		this->esp_status_label->setText("N.A.");
		this->esp_status_label->setStyleSheet(esp_status_label_style[0]);
		return;
	}
	bool status = (this->getNowMicroSec() - this->esp_status_map[key]) < secToMSec(5); // 5 sec timeout
	this->esp_status_label->setText(status ? "Online" : "Offline");
	this->esp_status_label->setStyleSheet(esp_status_label_style[status]);
}

void MainWindow::updateMQTTStatus(QMqttClient::ClientState state) {
	this->mqtt_state = state;
	switch (state) {
		case QMqttClient::ClientState::Disconnected: {
			mqtt_state_btn->setText("MQTT: Disconnected");
			mqtt_state_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case QMqttClient::ClientState::Connecting: {
			mqtt_state_btn->setText("MQTT: Connecting");
			mqtt_state_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case QMqttClient::ClientState::Connected: {
			mqtt_state_btn->setText("MQTT: Connected");
			mqtt_state_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
			break;
		}
		default: break;
	}
}

void MainWindow::updateData(const QByteArray& message, const QMqttTopicName& topic) {
	// esp/%s/d/%d
	// qDebug() << "Received data: " << message << " from topic: " << topic.name();
	bool need_update_index = this->data_map.isEmpty();

	QString key = topic.levels().at(1) + QString("_") + topic.levels().at(3);
	// qDebug() << "Received data from device: " << key;
	// qDebug() << "data_map contains? " << this->data_map.contains(key);

	// qint64 timestamp_ns = this->getNowMicroSec();

	// time_ms/t,x,y,z
	qint64 timestamp_ms = message.split('/').at(0).toLongLong();
	QList<QByteArray> data = message.split(',');
	if (data.size() != 4) {
		qDebug() << "Invalid data size";
		return;
	}
	int16_t T, X, Y, Z;
	T = data.at(0).toInt();
	X = data.at(1).toInt();
	Y = data.at(2).toInt();
	Z = data.at(3).toInt();

	if (this->recorder.getState() != RecorderStateReplaying) {
		this->recorder.dataRecord(key, timestamp_ms, T, X, Y, Z);
		this->processData(key, timestamp_ms, T, X, Y, Z);
	}
	this->updateEspStatus(topic.levels().at(1), true);
}

void MainWindow::processData(QString key, qint64 timestamp_ms, int16_t T, int16_t X, int16_t Y, int16_t Z) {
	// qDebug() << "Processing data: " << key << " " << timestamp_ms << " " << T << " " << X << " " << Y << " " << Z;

	bool need_reload_chart = false;
	if (this->data_clear_flags.contains(key) || timestamp_ms == 0) { // timestamp_ms == 0 for timer reset finish
		qDebug() << "Cal end, clearing data for device: " << key;
		if (this->data_map.contains(key)) {
			this->data_map[key]->clear();
		}
		this->data_clear_flags.remove(key);
		need_reload_chart = this->comboBox->currentText() == key;
	}

	const qreal time_sec = MSecToSec(timestamp_ms - this->start_time);
	// qDebug() << "Adding data to chart: " << time_sec << " " << X << " " << Y << " " << Z;

	if (time_sec != 0 && time_sec <= series[0]->points().last().x()) {
		// qDebug() << "Invalid time: " << time_sec << " " << series[0]->points().last().x();
		return;
	}

	if (!this->data_map.contains(key)) { // just on start
		bool is_left = true;
		Rotation rot = ROT_0;

		if (this->settings->contains(key)) {
			auto vars = this->settings->get(key).toArray();
			if (vars.size() == 3) { // for backward compatibility
				is_left = vars.at(2).toBool();
			} else if (vars.size() == 4) {
				is_left = vars.at(2).toBool();
				rot = (Rotation)vars.at(3).toInt();
			}
			int pos_x = vars.at(0).toInt(), pos_y = vars.at(1).toInt();
			this->sensor_is_left.insert(key, is_left);
			this->sensor_pos.insert(key, std::make_tuple(pos_x, pos_y));
			this->sensor_rot.insert(key, rot);
			this->graphicsManager->addSphereArrow(key, pos_x, pos_y, pos_x, pos_y, is_left);
			// qDebug() << "pt1: " << this->sensor_is_left[key] << " " << std::get<0>(this->sensor_pos[key]) << " "
			// 		 << std::get<1>(this->sensor_pos[key]);
		} else {
			this->sensor_is_left.insert(key, true);
			this->sensor_pos.insert(key, std::make_tuple(0, 0));
			this->sensor_rot.insert(key, ROT_0);
			this->graphicsManager->addSphereArrow(key, 0, 0, 0, 0, true);
		}

		if (!is_left) {
			X = -X;
			Y = -Y;
		}

		this->data_map.insert(key, new DataContainer(data_series_size));
		this->data_map[key]->append(timestamp_ms, X, Y, Z);

		this->comboBox->addItem(key);
		this->comboBox->model()->sort(0);

		if (this->comboBox->currentText() == key) {
			if (this->sensor_is_left.contains(key)) {
				if (this->sensor_is_left[key]) {
					this->xy_left_btn->setChecked(true);
				} else {
					this->xy_right_btn->setChecked(true);
				}
			} else {
				this->xy_left_btn->setChecked(true);
			}
			// qDebug() << "pt2: " << this->sensor_is_left[key] << " " << std::get<0>(this->sensor_pos[key]) << " "
			// 		 << std::get<1>(this->sensor_pos[key]);
			this->x_input->setText(QString::number(std::get<0>(this->sensor_pos[key])));
			this->y_input->setText(QString::number(std::get<1>(this->sensor_pos[key])));
		}

		qDebug() << "new device added: " << key;
	} else {
		if (!this->sensor_is_left[key]) {
			X = -X;
			Y = -Y;
		}
		switch (this->sensor_rot[key]) { // clockwise
			case ROT_90: {
				int tmp = X;
				X = Y;
				Y = -tmp;
				break;
			}
			case ROT_180: {
				X = -X;
				Y = -Y;
				break;
			}
			case ROT_270: {
				int tmp = X;
				X = -Y;
				Y = tmp;
				break;
			}
			default: break;
		}

		this->data_map[key]->append(timestamp_ms, X, Y, Z);
		if (this->comboBox->currentText() == key) {
			this->addChartData(timestamp_ms, X, Y, Z);
		}
	}

	if (need_reload_chart) {
		qDebug() << "Reloading chart due to recalibration";
		this->updateChartSelect(this->comboBox->currentIndex());
	}

	// static qint64 last_update = 0;
	// if (this->getNowMicroSec() - last_update < 20) { // 100ms update interval
	// 	return;
	// }
	// last_update = this->getNowMicroSec();

	const qreal scale = 600;
	// this->graphicsManager->setArrowPointingToScalar(key, X / scale, Y / scale);
	// this->graphicsManager->setDefaultSphereColorScalar(key, Z / scale);
	// this->graphicsManager->setArrowRot90(key, this->sensor_rot[key]);
	this->graphics_mutex.lock();
	if (!this->graphics_data.contains(key)) {
		this->graphics_data.insert(key, std::make_tuple(0.0, 0.0, 0.0));
		this->graphics_data_num.insert(key, 0);
	}
	auto data = this->graphics_data[key];
	auto num = this->graphics_data_num[key];
	std::get<0>(data) += X / scale;
	std::get<1>(data) += Y / scale;
	std::get<2>(data) += Z / scale;
	this->graphics_data[key] = data;
	this->graphics_data_num[key] = num + 1;
	this->graphics_mutex.unlock();
}

// void MainWindow::updateGraphicsData() {
// 	this->graphics_mutex.lock();
// 	for (auto key : this->graphics_data.keys()) {
// 		auto data = this->graphics_data[key];
// 		auto num = this->graphics_data_num[key];
// 		if (num > 0) {
// 			this->graphicsManager->setArrowPointingToScalar(key, std::get<0>(data) / num, std::get<1>(data) / num);
// 			this->graphicsManager->setDefaultSphereColorScalar(key, std::get<2>(data) / num);
// 		}
// 		this->graphics_data[key] = std::make_tuple(0.0, 0.0, 0.0);
// 		this->graphics_data_num[key] = 0;
// 	}
// 	this->graphics_mutex.unlock();
// }

void MainWindow::updateCalEndStatus(const QString esp_id, const QString sensor_id) {
	qDebug() << "Calibration end received: " << esp_id << "::" << sensor_id;

	// set data clear flag
	this->data_clear_flags.insert(esp_id + QString("_") + sensor_id);
}

void MainWindow::updateChartSelect(int index) {
	qDebug() << "Sensor chart index changed to " << index;

	/* !===== for testing =====! */
	// series[0]->append(series[0]->points().size(), index + 1);
	// series[1]->append(series[1]->points().size(), index + 10);
	// series[2]->append(series[2]->points().size(), index + 1000);
	/* !======================! */

	QString key = this->comboBox->itemText(index);
	qDebug() << "Selected device: " << key;
	DataContainer* data = this->data_map[key];
	if (!data) {
		qDebug() << "Data not found";
		return;
	}
	// qDebug() << "Data size: " << data->size();
	data_queue_mutex.lock();
	data_queue.clear();
	data_queue_mutex.unlock();

	series[0]->clear();
	series[1]->clear();
	series[2]->clear();
	chart_data[0].clear();
	chart_data[1].clear();
	chart_data[2].clear();
	qDebug() << "Series cleared";

	for (auto [timestamp_ms, value] : *data) {
		const qreal time_sec = MSecToSec(timestamp_ms - this->start_time);
		// series[0]->append(time_sec, std::get<0>(value));
		// series[1]->append(time_sec, std::get<1>(value));
		// series[2]->append(time_sec, std::get<2>(value));
		chart_data[0].append((QPointF){time_sec, (qreal)std::get<0>(value)});
		chart_data[1].append((QPointF){time_sec, (qreal)std::get<1>(value)});
		chart_data[2].append((QPointF){time_sec, (qreal)std::get<2>(value)});
	}
	if (chart_data[0].size() != 0) {
		series[0]->replace(chart_data[0]);
		series[1]->replace(chart_data[1]);
		series[2]->replace(chart_data[2]);
	}
	if (this->esp_status_map.contains(key) && this->getNowMicroSec() - this->esp_status_map[key] < secToMSec(5)) {
		this->esp_status_label->setText("Online");
		this->esp_status_label->setStyleSheet(esp_status_label_style[1]);
	} else {
		this->esp_status_label->setText("Offline");
		this->esp_status_label->setStyleSheet(esp_status_label_style[0]);
	}

	if (this->sensor_pos.contains(key)) {
		this->x_input->setText(QString::number(std::get<0>(this->sensor_pos[key])));
		this->y_input->setText(QString::number(std::get<1>(this->sensor_pos[key])));
	} else {
		this->x_input->setText("");
		this->y_input->setText("");
	}
	if (this->sensor_is_left.contains(key)) {
		if (this->sensor_is_left[key]) {
			this->xy_left_btn->setChecked(true);
		} else {
			this->xy_right_btn->setChecked(true);
		}
	} else {
		this->xy_left_btn->setChecked(true);
	}

	qDebug() << "Data appended, reloading chart";

	this->reloadChart();
}

void MainWindow::reloadChart() {
	qDebug() << "Updating chart";

	for (int i = 0; i < 3; i++) {
		qDebug() << "i: " << i << " series size: " << series[i]->points().size();
		qreal minX = 0, maxX = 0, minY = 0, maxY = 0;
		for (const QPointF& point : series[i]->points()) {
			// qDebug() << "point: " << point.x() << " " << point.y();
			if (point == series[i]->points().first()) {
				minX = maxX = point.x();
				minY = maxY = point.y();
			} else {
				minX = qMin(minX, point.x());
				maxX = qMax(maxX, point.x());
				minY = qMin(minY, point.y());
				maxY = qMax(maxY, point.y());
			}
		}
		// qDebug() << "minX: " << minX << " maxX: " << maxX << " minY: " << minY << " maxY: " << maxY;
		if (abs(maxX - minX) < 1) {
			maxX += 1;
		}
		if (abs(maxY - minY) < 1) {
			maxY += 1;
		}

		chart_range_y[i] = std::make_tuple(minY, maxY);

		chart[i]->axes(Qt::Horizontal).back()->setRange(minX, maxX);
		const qreal padding = ceil((maxY - minY) * 0.2);
		chart[i]->axes(Qt::Vertical).back()->setRange(minY - padding, maxY + padding);

		chart[i]->update();
		chartView[i]->update();
		qDebug() << "Chart " << i << " updated";
		qDebug() << "x range: " << minX << " " << maxX;
		qDebug() << "y range: " << minY << " " << maxY;
	}
	qDebug() << series[0]->points().size() << " " << series[1]->points().size() << " " << series[2]->points().size();
}


void MainWindow::addChartData(qint64 timestamp_ms, int16_t X, int16_t Y, int16_t Z) {
	const qreal time_sec = MSecToSec(timestamp_ms - this->start_time);
	// qDebug() << "Adding data to chart: " << time_sec << " " << X << " " << Y << " " << Z;

	data_queue_mutex.lock();
	data_queue.enqueue({time_sec, (qreal)X, (qreal)Y, (qreal)Z});
	is_data_queue_updated = true;
	data_queue_mutex.unlock();
}

// void MainWindow::updateChartData() {
// 	data_queue_mutex.lock();
// 	if (!is_data_queue_updated) {
// 		data_queue_mutex.unlock();
// 		return;
// 	}
// 	is_data_queue_updated = false;

// 	for (const DataPoint& data : data_queue) {
// 		// series[0]->append(data.time_sec, data.X);
// 		// series[1]->append(data.time_sec, data.Y);
// 		// series[2]->append(data.time_sec, data.Z);
// 		chart_data[0].append((QPointF){data.time_sec, data.X});
// 		chart_data[1].append((QPointF){data.time_sec, data.Y});
// 		chart_data[2].append((QPointF){data.time_sec, data.Z});
// 		qreal d[3] = {data.X, data.Y, data.Z};
// 		for (int i = 0; i < 3; i++) {
// 			if (chart_data[i].size() > data_series_size) {
// 				chart_data[i].removeFirst();
// 			}

// 			const qreal minY = qMin(qMin(std::get<0>(chart_range_y[i]), d[i]), std::get<0>(chart_range_y[i]));
// 			const qreal maxY = qMax(qMax(std::get<1>(chart_range_y[i]), d[i]), std::get<1>(chart_range_y[i]));
// 			chart_range_y[i] = std::make_tuple(minY, maxY);
// 		}
// 	}
// 	data_queue.clear();
// 	data_queue_mutex.unlock();

// 	if (chart_data[0].size() == 0) {
// 		return;
// 	}

// 	chartView[0]->setUpdatesEnabled(false);
// 	chartView[1]->setUpdatesEnabled(false);
// 	chartView[2]->setUpdatesEnabled(false);

// 	series[0]->clear();
// 	series[1]->clear();
// 	series[2]->clear();
// 	series[0]->replace(chart_data[0]);
// 	series[1]->replace(chart_data[1]);
// 	series[2]->replace(chart_data[2]);

// 	chartView[0]->setUpdatesEnabled(true);
// 	chartView[1]->setUpdatesEnabled(true);
// 	chartView[2]->setUpdatesEnabled(true);

// 	for (int i = 0; i < 3; i++) {
// 		qreal minX = series[i]->points().first().x();
// 		qreal maxX = series[i]->points().last().x();
// 		// qreal minY = qMin(qMin(std::get<0>(chart_range_y[i]), d[i]), std::get<0>(chart_range_y[i]));
// 		// qreal maxY = qMax(qMax(std::get<1>(chart_range_y[i]), d[i]), std::get<1>(chart_range_y[i]));

// 		// chart_range_y[i] = std::make_tuple(minY, maxY);
// 		qreal minY = std::get<0>(chart_range_y[i]);
// 		qreal maxY = std::get<1>(chart_range_y[i]);

// 		chart[i]->axes(Qt::Horizontal).back()->setRange(minX, maxX);
// 		const qreal padding = ceil((maxY - minY) * 0.2);
// 		chart[i]->axes(Qt::Vertical).back()->setRange(minY - padding, maxY + padding);

// 		chart[i]->update();
// 		chartView[i]->update();
// 		// qDebug() << "Chart " << i << " appended to size: " << series[i]->points().size();
// 		// qDebug() << "x range: " << minX << " " << maxX;
// 		// qDebug() << "y range: " << minY << " " << maxY;
// 	}
// }

void MainWindow::xySaveButtonClicked() {
	qDebug() << "Save button clicked";
	bool ok;
	int x = x_input->text().toInt(&ok);
	x = qBound(0, x, this->graphicsManager->width() / 2);
	if (!ok) {
		qDebug() << "Invalid X input";
		return;
	}
	int y = y_input->text().toInt(&ok);
	y = qBound(0, y, this->graphicsManager->height());
	if (!ok) {
		qDebug() << "Invalid Y input";
		return;
	}
	auto key = this->comboBox->currentText();

	bool is_left = this->xy_left_btn->isChecked();
	bool is_left_changed = (is_left != this->sensor_is_left[key]);
	this->graphicsManager->setSpherePos(key, x, y, is_left, is_left_changed);

	Rotation rot = this->sensor_rot[key];

	QJsonArray arr = {x, y, is_left, (int)rot};
	this->settings->set(key, arr);
	this->settings->save();

	this->sensor_is_left[key] = is_left;
	this->sensor_pos[key] = std::make_tuple(x, y);

	// change all data in data_map, and if current device is selected, update chart
	if (is_left_changed) {
		DataContainer* data = this->data_map[key];
		if (data) {
			for (auto [timestamp_ms, value] : *data) {
				auto [X, Y, Z] = value;
				X = -X;
				Y = -Y;
			}
			this->updateChartSelect(this->comboBox->currentIndex());
		}
	}
}

void MainWindow::sensorRecalibrationButtonClicked() {
	qDebug() << "Recalibration button clicked";

	// mqtt publish to app/cal/{esp_id}/{sensor_id}

	// construct topic
	for (int i = 0; i < this->comboBox->count(); i++) {
		auto args = this->comboBox->itemText(i).split('_');
		if (args.size() < 2) {
			qDebug() << "Invalid device name";
			continue;
		}
		QString esp_id = "";
		for (int i = 0; i < args.size() - 1; i++) {
			esp_id += args.at(i);
			if (i != args.size() - 2) {
				esp_id += "_";
			}
		}
		QString topic = "app/cal/%1/%2";
		topic = topic.arg(esp_id).arg(args.last());

		// publish
		this->mqtt->publish(QByteArray(), QMqttTopicName(topic));

		// current data will be cleared when calEndReceived signal is received
	}
	// popout dialog
	showInfoBox("Recalibration request sent");
}

void MainWindow::rotButtonClicked() {
	qDebug() << "Rotate button clicked";
	QString current_device = this->comboBox->currentText();
	if (!this->sensor_rot.contains(current_device)) {
		qDebug() << "Invalid device";
		return;
	}
	this->sensor_rot[current_device] = (Rotation)((this->sensor_rot[current_device] + 1) % NUM_OF_ROTATION);
	this->graphicsManager->setArrowRot90(current_device, 1);
	this->xySaveButtonClicked(); // save everything
}

/* replay related */
void MainWindow::mqttStateBtnClicked() {
	qDebug() << "MQTT state button clicked";
	if (this->recorder.getState() != RecorderStateIdle) {
		qDebug() << "Recorder state not idle, cannot start replay";
		return;
	}
	// start replay
	QDir dir = QDir::current();
	if (dir.exists("recordings")) {
		dir.cd("recordings");
	}
	QString path = QFileDialog::getOpenFileName(this, "Open recording file", dir.absolutePath(), "JSON files (*.json)");
	if (path.isEmpty()) {
		qDebug() << "No file selected";
		return;
	}
	if (!this->recorder.startReplaying(path)) {
		qDebug() << "Failed to start replay";
		return;
	}
	this->start_stop_btn->setText("Stop replay");
	this->start_stop_btn->setStyleSheet(
		"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
}

void MainWindow::startStopBtnClicked() {
	qDebug() << "Start/Stop button clicked";
	switch (this->recorder.getState()) {
		case RecorderStateIdle: {
			// start recording
			if (!this->recorder.startRecording()) {
				qDebug() << "Failed to start recording";
			}
			this->start_stop_btn->setText("Stop record");
			this->start_stop_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			// this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
			break;
		}
		case RecorderStateRecording: {
			// stop recording
			if (!this->recorder.stopRecording()) {
				qDebug() << "Failed to stop recording";
			}
			this->start_stop_btn->setText("Start record");
			this->start_stop_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
			break;
		}
		case RecorderStateReplaying: {
			// stop replaying
			if (!this->recorder.stopReplaying()) {
				qDebug() << "Failed to stop replaying";
			}
			this->start_stop_btn->setText("Start record");
			this->start_stop_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
			this->clear();
			break;
		}
		default: break;
	}
}

void MainWindow::replayFinished() {
	qDebug() << "replay finished sig recv";
	if (this->recorder.getState() != RecorderStateReplaying) {
		qDebug() << "Recorder state not replaying, no action needed";
		return;
	}
	// this->start_stop_btn->setText("Stop replay");
	this->start_stop_btn->setStyleSheet(
		"QPushButton { border-radius: 5px; background-color: #a9a9a9; color:rgb(164, 134, 0); }");
}

void MainWindow::updateEspStatus(const QString esp_id, bool status) {
	// qDebug() << "Updating ESP status: " << esp_id << " " << status;
	this->esp_status_map[esp_id] = this->getNowMicroSec();
	if (this->comboBox->currentText().startsWith(esp_id)) {
		this->esp_status_label->setText(status ? "Online" : "Offline");
		this->esp_status_label->setStyleSheet(esp_status_label_style[status]);
	}
}

void MainWindow::clear() {
	qDebug() << "Clearing data";
	for (auto value : this->data_map.values()) {
		delete value;
	}
	this->data_map.clear();
	this->comboBox->clear();
	this->graphicsManager->clear();
	this->reloadChart();
	this->elapsed_timer.restart();
	this->start_time = this->getNowMicroSec();
}
