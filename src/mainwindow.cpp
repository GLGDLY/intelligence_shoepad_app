#include "mainwindow.h"

#include "./ui_mainwindow.h"

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
	, mqtt_state_label(new QLabel())
	, mqtt_state(QMqttClient::ClientState::Disconnected)
	, mqtt_last_received(0)
	, mqtt_last_received_timer(new QTimer())
	, mqtt(new MqttApp())
	, x_input(new QLineEdit())
	, y_input(new QLineEdit())
	, xy_save_button(new QPushButton("Save"))
	, settings(new Settings()) {
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

		chart[i]->axes(Qt::Horizontal).back()->setTitleText("Time");
		chart[i]->axes(Qt::Horizontal).back()->setTitleFont(QFont("Arial", 10));
		chart[i]->axes(Qt::Horizontal).back()->setMinorGridLineVisible(false);
		chart[i]->axes(Qt::Horizontal).back()->setGridLineVisible(false);

		chart[i]->axes(Qt::Vertical).back()->setTitleFont(QFont("Arial", 10));
		chart[i]->axes(Qt::Vertical).back()->setMinorGridLineVisible(false);
		chart[i]->axes(Qt::Vertical).back()->setGridLineVisible(false);

		chart[i]->addAxis(chart[i]->axes(Qt::Horizontal).back(), Qt::AlignBottom);
		chart[i]->addAxis(chart[i]->axes(Qt::Vertical).back(), Qt::AlignLeft);

		switch (i) {
			case 0:
				series[i]->setColor(Qt::red);
				chart[i]->axes(Qt::Vertical).back()->setTitleText("Value X");
				break;
			case 1:
				series[i]->setColor(Qt::green);
				chart[i]->axes(Qt::Vertical).back()->setTitleText("Value Y");
				break;
			case 2:
				series[i]->setColor(Qt::blue);
				chart[i]->axes(Qt::Vertical).back()->setTitleText("Value Z");
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

	// mqtt status bar
	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(5);
	effect->setOffset(2, 2);
	effect->setColor(QColor(0, 0, 0, 255 * 0.1));
	mqtt_state_label->setGraphicsEffect(effect);

	mqtt_state_label->setText("MQTT: Disconnected");
	mqtt_state_label->setAlignment(Qt::AlignCenter);
	mqtt_state_label->setTextFormat(Qt::TextFormat::RichText);
	mqtt_state_label->setFont(QFont("Calibri", 11, QFont::Medium));
	mqtt_state_label->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
	mqtt_state_label->setGeometry(10, this->height() - 25 - 10, 220, 25);
	this->layout()->addWidget(mqtt_state_label);

	// mqtt
	mqtt->connect_client_signal(&QMqttClient::stateChanged, this, &MainWindow::updateMQTTStatus);
	connect(mqtt, &MqttApp::dataReceived, this, &MainWindow::updateData);

	// mqtt timer
	connect(mqtt_last_received_timer, &QTimer::timeout, this, &MainWindow::updateMQTTLastReceived);
	mqtt_last_received_timer->start(1000);
}

MainWindow::~MainWindow() {
	delete ui;
	for (int i = 0; i < 3; i++) {
		delete chartView[i];
		delete chart[i];
		delete series[i];
	}
	delete comboBox;
	delete mqtt_state_label;
	delete mqtt;
	for (auto value : this->data_map.values()) {
		delete value;
	}
	if (this->graphicsManager) {
		delete this->graphicsManager;
	}
}

void MainWindow::updateMQTTLastReceived() {
	// static bool do_once = true;
	// if (do_once) {
	// 	this->updateData(QByteArray("1,2,3,4"), QMqttTopicName("esp/test_esp/d/0"));
	// 	do_once = false;
	// }

	if (this->mqtt_state == QMqttClient::ClientState::Connected) {
		QString text = "MQTT: Connected(%1)";
		text = text.arg((QDateTime::currentMSecsSinceEpoch() - this->mqtt_last_received) / 1000);
		mqtt_state_label->setText(text);
	}
}

void MainWindow::updateMQTTStatus(QMqttClient::ClientState state) {
	this->mqtt_state = state;
	switch (state) {
		case QMqttClient::ClientState::Disconnected: {
			mqtt_state_label->setText("MQTT: Disconnected");
			mqtt_state_label->setStyleSheet(
				"QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case QMqttClient::ClientState::Connecting: {
			mqtt_state_label->setText("MQTT: Connecting");
			mqtt_state_label->setStyleSheet(
				"QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case QMqttClient::ClientState::Connected: {
			this->mqtt_last_received = QDateTime::currentMSecsSinceEpoch();
			mqtt_state_label->setText("MQTT: Connected(0)");
			mqtt_state_label->setStyleSheet(
				"QLabel { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
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

	const qreal scale = 600;
	this->graphicsManager->setArrowPointingToScalar(key, X / scale, Y / scale);
	this->graphicsManager->setDefaultSphereColorScalar(key, Z / scale);
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
	// qDebug() << "Data size: " << data->size();
	series[0]->clear();
	series[1]->clear();
	series[2]->clear();
	qDebug() << "Series cleared";

	for (auto [timestamp, value] : *data) {
		series[0]->append(timestamp, std::get<0>(value));
		series[1]->append(timestamp, std::get<1>(value));
		series[2]->append(timestamp, std::get<2>(value));
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
		qreal minX, maxX, minY, maxY;
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
