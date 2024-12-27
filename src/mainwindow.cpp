#include "mainwindow.h"

#include "./ui_mainwindow.h"
#include "data_recorder.hpp"
#include "infobox.hpp"

#include <QFileDialog>
#include <QGraphicsEffect>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QValueAxis>
#include <QtLogging>
#include <QtMinMax>


MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, comboBox(new QComboBox())
	, chartView{new QChartView(), new QChartView(), new QChartView()}
	, chart{new QChart(), new QChart(), new QChart()}
	, series{new QSplineSeries(), new QSplineSeries(), new QSplineSeries()}
	, mqtt_state_btn(new QPushButton())
	, start_stop_btn(new QPushButton())
	, mqtt_state(QMqttClient::ClientState::Disconnected)
	, mqtt_last_received(0)
	, mqtt_last_received_timer(new QTimer())
	, mqtt(new MqttApp())
	, x_input(new QLineEdit())
	, y_input(new QLineEdit())
	, xy_save_button(new QPushButton("Save"))
	, sensor_recalibration_button(new QPushButton("Recalibrate"))
	, data_clear_flags()
	, settings(new Settings())
	, start_time(QDateTime::currentMSecsSinceEpoch()) {
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
	comboBox->setGeometry(450 + 10, 10, this->width() - 450 - 20, 25);
	this->layout()->addWidget(comboBox);

	// chart
	auto chart_height = (this->height() - comboBox->height() - comboBox->y()) / 3;
	for (int i = 0; i < 3; i++) {
		chart[i]->setTheme(QChart::ChartThemeHighContrast);
		chart[i]->setAnimationOptions(QChart::AllAnimations);
		chart[i]->setAnimationDuration(50);
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

		chart[i]->addSeries(series[i]);
		chart[i]->createDefaultAxes();

		QValueAxis* axisX = (QValueAxis*)chart[i]->axes(Qt::Horizontal).back();
		axisX->setTitleText("Time");
		axisX->setTitleFont(QFont("Arial", 10));
		axisX->setMinorGridLineVisible(false);
		axisX->setGridLineVisible(false);
		axisX->setLabelFormat("%d");

		QValueAxis* axisY = (QValueAxis*)chart[i]->axes(Qt::Vertical).back();
		axisY->setTitleFont(QFont("Arial", 10));
		axisY->setMinorGridLineVisible(false);
		axisY->setGridLineVisible(false);
		axisY->setLabelFormat("%d");

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

		chartView[i]->setRenderHint(QPainter::Antialiasing);
		chartView[i]->setGeometry(450, comboBox->height() + comboBox->y() + i * chart_height, this->width() - 450,
								  chart_height);
		chartView[i]->setChart(chart[i]);
		this->layout()->addWidget(chartView[i]);
	}
	this->reloadChart();

	// xy input box
	QString x_placeholder = "X: 0-%1";
	QString y_placeholder = "Y: 0-%1";
	x_input->setPlaceholderText(x_placeholder.arg(this->graphicsManager->width()));
	x_input->setGeometry(10, 10, 100, 25);
	x_input->setStyleSheet("QLineEdit { background-color:rgb(202, 202, 202); color: #000; }");
	this->layout()->addWidget(x_input);

	y_input->setPlaceholderText(y_placeholder.arg(this->graphicsManager->height()));
	y_input->setGeometry(110, 10, 100, 25);
	y_input->setStyleSheet("QLineEdit { background-color: rgb(202, 202, 202); color: #000; }");
	this->layout()->addWidget(y_input);

	// xy save button
	xy_save_button->setGeometry(210, 10, 50, 25);
	xy_save_button->setStyleSheet("QPushButton { background-color: #a9a9a9; color: #000; }");
	this->layout()->addWidget(xy_save_button);
	connect(xy_save_button, &QPushButton::clicked, this, &MainWindow::xySaveButtonClicked);

	// sensor recalibration button
	sensor_recalibration_button->setGeometry(270, 10, 100, 25);
	sensor_recalibration_button->setStyleSheet("QPushButton { background-color: #a9a9a9; color: #000; }");
	this->layout()->addWidget(sensor_recalibration_button);
	connect(sensor_recalibration_button, &QPushButton::clicked, this, &MainWindow::sensorRecalibrationButtonClicked);

	// mqtt status bar
	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(5);
	effect->setOffset(2, 2);
	effect->setColor(QColor(0, 0, 0, 255 * 0.1));
	mqtt_state_btn->setGraphicsEffect(effect);
	mqtt_state_btn->setText("MQTT: Disconnected");
	mqtt_state_btn->setFont(QFont("Calibri", 11, QFont::Medium));
	mqtt_state_btn->setStyleSheet("QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
	mqtt_state_btn->setGeometry(10, this->height() - 25 - 10, 220, 30);
	connect(mqtt_state_btn, &QPushButton::clicked, this, &MainWindow::mqttStateBtnClicked);
	this->layout()->addWidget(mqtt_state_btn);

	// start_stop_btn
	start_stop_btn->setGraphicsEffect(effect);
	start_stop_btn->setText("Start record");
	start_stop_btn->setFont(QFont("Calibri", 11, QFont::Medium));
	start_stop_btn->setStyleSheet("QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
	start_stop_btn->setGeometry(240, this->height() - 25 - 10, 100, 30);
	connect(start_stop_btn, &QPushButton::clicked, this, &MainWindow::startStopBtnClicked);
	this->layout()->addWidget(start_stop_btn);

	// mqtt
	mqtt->connect_client_signal(&QMqttClient::stateChanged, this, &MainWindow::updateMQTTStatus);
	connect(mqtt, &MqttApp::dataReceived, this, &MainWindow::updateData);
	connect(mqtt, &MqttApp::calEndReceived, this, &MainWindow::updateCalEndStatus);

	// mqtt timer
	connect(mqtt_last_received_timer, &QTimer::timeout, this, &MainWindow::updateMQTTLastReceived);
	mqtt_last_received_timer->start(1000);

	// recorder
	connect(&recorder, &DataRecorder::playbackData, this, &MainWindow::processData);
	connect(&recorder, &DataRecorder::replayStarted, this, &MainWindow::clear);
	connect(&recorder, &DataRecorder::replayFinished, this, &MainWindow::replayFinished);
}

MainWindow::~MainWindow() {
	delete ui;
	for (int i = 0; i < 3; i++) {
		delete chartView[i];
		delete chart[i];
		delete series[i];
	}
	delete comboBox;
	delete mqtt_state_btn;
	delete mqtt;
	for (auto value : this->data_map.values()) {
		delete value;
	}
	if (this->graphicsManager) {
		delete this->graphicsManager;
	}
}

void MainWindow::updateMQTTLastReceived() {
	/* Testing */
	// static bool do_once = true;
	// if (do_once) {
	// 	this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
	// 	this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
	// 	this->updateCalEndStatus("test_esp", "0");
	// 	this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
	// 	this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
	// 	do_once = false;
	// }
	/* Testing */

	if (this->mqtt_state == QMqttClient::ClientState::Connected) {
		QString text = "MQTT: Connected(%1)";
		text = text.arg((QDateTime::currentMSecsSinceEpoch() - this->mqtt_last_received) / 1000);
		mqtt_state_btn->setText(text);
	}
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
			this->mqtt_last_received = QDateTime::currentMSecsSinceEpoch();
			mqtt_state_btn->setText("MQTT: Connected(0)");
			mqtt_state_btn->setStyleSheet(
				"QPushButton { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
			break;
		}
		default: break;
	}
}

void MainWindow::updateData(const QByteArray& message, const QMqttTopicName& topic) {
	// esp/%s/d/%d
	qDebug() << "Received data: " << message << " from topic: " << topic.name();
	bool need_update_index = this->data_map.isEmpty();

	QString key = topic.levels().at(1) + QString("_") + topic.levels().at(3);
	// qDebug() << "Received data from device: " << key;
	// qDebug() << "data_map contains? " << this->data_map.contains(key);

	qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
	this->mqtt_last_received = timestamp;
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
		this->recorder.dataRecord(key, timestamp, T, X, Y, Z);
		this->processData(key, timestamp, T, X, Y, Z);
	}
}

void MainWindow::processData(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z) {
	qDebug() << "Processing data: " << key << " " << timestamp << " " << T << " " << X << " " << Y << " " << Z;

	bool need_reload_chart = false;
	if (this->data_clear_flags.contains(key)) {
		qDebug() << "Cal end, clearing data for device: " << key;
		if (this->data_map.contains(key)) {
			this->data_map[key]->clear();
		}
		this->data_clear_flags.remove(key);
		need_reload_chart = this->comboBox->currentText() == key;
	}

	if (!this->data_map.contains(key)) {
		this->data_map.insert(key, new DataContainer(1000));
		this->data_map[key]->append(timestamp, X, Y, Z);

		this->comboBox->addItem(key);
		this->comboBox->model()->sort(0);

		if (this->settings->contains(key)) {
			auto vars = this->settings->get(key).toArray();
			this->graphicsManager->addSphereArrow(key, vars.at(0).toInt(), vars.at(1).toInt(), X, Y);
		} else {
			this->graphicsManager->addSphereArrow(key, 0, 0, 0, 0);
		}

		qDebug() << "new device added: " << key;
	} else {
		this->data_map[key]->append(timestamp, X, Y, Z);
		if (this->comboBox->currentText() == key) {
			this->addChartData(timestamp, X, Y, Z);
		}
	}

	if (need_reload_chart) {
		qDebug() << "Reloading chart due to recalibration";
		this->updateChartSelect(this->comboBox->currentIndex());
	}

	const qreal scale = 600;
	this->graphicsManager->setArrowPointingToScalar(key, X / scale, Y / scale);
	this->graphicsManager->setDefaultSphereColorScalar(key, Z / scale);
}

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
	series[0]->clear();
	series[1]->clear();
	series[2]->clear();
	qDebug() << "Series cleared";

	for (auto [timestamp, value] : *data) {
		series[0]->append(timestamp - this->start_time, std::get<0>(value));
		series[1]->append(timestamp - this->start_time, std::get<1>(value));
		series[2]->append(timestamp - this->start_time, std::get<2>(value));
	}
	// for (auto s : series) {
	// 	// qDebug() << "Series size: " << s->points().size();
	// 	if (s->points().size() == 1) {
	// 		series[0]->append(series[0]->points().first().x(), series[0]->points().first().y());
	// 	}
	// }

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

void MainWindow::addChartData(qint64 timestamp, int16_t X, int16_t Y, int16_t Z) {
	timestamp -= this->start_time;
	qDebug() << "Adding data to chart: " << timestamp << " " << X << " " << Y << " " << Z;

	series[0]->append(timestamp, X);
	series[1]->append(timestamp, Y);
	series[2]->append(timestamp, Z);

	const qreal d[3] = {(qreal)X, (qreal)Y, (qreal)Z};
	for (int i = 0; i < 3; i++) {
		if (series[i]->points().size() > 1000) {
			series[i]->remove(0);
		}

		qreal minX = series[i]->points().first().x();
		qreal maxX = timestamp;
		qreal minY = qMin(std::get<0>(chart_range_y[i]), d[i]);
		qreal maxY = qMax(std::get<1>(chart_range_y[i]), d[i]);

		chart_range_y[i] = std::make_tuple(minY, maxY);

		chart[i]->axes(Qt::Horizontal).back()->setRange(minX, maxX);
		const qreal padding = ceil((maxY - minY) * 0.2);
		chart[i]->axes(Qt::Vertical).back()->setRange(minY - padding, maxY + padding);

		chart[i]->update();
		chartView[i]->update();
		qDebug() << "Chart " << i << " appended to size: " << series[i]->points().size();
		qDebug() << "x range: " << minX << " " << maxX;
		qDebug() << "y range: " << minY << " " << maxY;
	}
}

void MainWindow::xySaveButtonClicked() {
	qDebug() << "Save button clicked";
	bool ok;
	int x = x_input->text().toInt(&ok);
	if (!ok) {
		qDebug() << "Invalid X input";
		return;
	}
	int y = y_input->text().toInt(&ok);
	if (!ok) {
		qDebug() << "Invalid Y input";
		return;
	}
	auto key = this->comboBox->currentText();
	this->graphicsManager->setSpherePos(key, fmin(fmax(x, 0), this->graphicsManager->width()),
										fmin(fmax(y, 0), this->graphicsManager->height()));
	QJsonArray arr = {x, y};
	this->settings->set(key, arr);
	this->settings->save();
}

void MainWindow::sensorRecalibrationButtonClicked() {
	qDebug() << "Recalibration button clicked";

	// mqtt publish to app/cal/{esp_id}/{sensor_id}

	// construct topic
	auto args = this->comboBox->currentText().split('_');
	if (args.size() < 2) {
		qDebug() << "Invalid device name";
		return;
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

	// popout dialog
	showInfoBox("Recalibration request sent");

	// current data will be cleared when calEndReceived signal is received
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

void MainWindow::clear() {
	qDebug() << "Clearing data";
	for (auto value : this->data_map.values()) {
		delete value;
	}
	this->data_map.clear();
	this->comboBox->clear();
	this->graphicsManager->clear();
	this->reloadChart();
	this->start_time = QDateTime::currentMSecsSinceEpoch();
}
