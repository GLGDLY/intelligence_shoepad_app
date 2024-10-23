#include "mainwindow.h"

#include "./ui_mainwindow.h"

#include <QGraphicsEffect>
#include <QPixmap>
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
	, mqtt_status(new QLabel()) {
	ui->setupUi(this);

	this->setWindowTitle(tr("Intelligence Shoepad"));
	this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);
	this->setFixedSize(this->size());
	this->setStyleSheet("QMainWindow { background-color: #f0f0f0; }");

	// graphicsView
	this->ui->graphicsView->setStyleSheet("QGraphicsView { border: 0px solid #000; }");
	auto img = QPixmap("images/insole.jpg");
	img = img.scaled(this->ui->graphicsView->size(), Qt::KeepAspectRatio);

	// set image to rounded corners
	QBitmap mask(img.size());
	QPainter painter(&mask);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(img.rect(), Qt::white);
	painter.setBrush(Qt::black);
	painter.drawRoundedRect(img.rect(), 10, 10);
	painter.end();
	img.setMask(mask);

	// set image to graphicsView
	this->ui->graphicsView->setScene(new QGraphicsScene(this));
	this->ui->graphicsView->scene()->addPixmap(img);

	// comboBox
	comboBox->setInsertPolicy(QComboBox::InsertAtBottom);
	comboBox->setStyle(QStyleFactory::create("Fusion"));
	comboBox->addItems({"1", "2", "3"});
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
	this->updateChart();

	// mqtt status bar
	QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(5);
	effect->setOffset(2, 2);
	effect->setColor(QColor(0, 0, 0, 255 * 0.1));
	mqtt_status->setGraphicsEffect(effect);

	mqtt_status->setText("MQTT: Disconnected");
	mqtt_status->setAlignment(Qt::AlignCenter);
	mqtt_status->setTextFormat(Qt::TextFormat::RichText);
	mqtt_status->setFont(QFont("Calibri", 11, QFont::Medium));
	mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
	mqtt_status->setGeometry(10, this->height() - 25 - 10, 220, 25);
	// connect(m_sub, &QMqttSubscription::stateChanged, this, &MainWindow::updateMQTTStatus);
	this->layout()->addWidget(mqtt_status);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::updateMQTTStatus(QMqttSubscription::SubscriptionState state) {
	switch (state) {
		case 0: { // QMqttSubscription::Unsubscribed
			mqtt_status->setText("MQTT: Disconnected");
			mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case 1: { // QMqttSubscription::SubscriptionPending (not yet confirmed by the broker)
			mqtt_status->setText("MQTT: SubscriptionPending");
			mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case 2: { // QMqttSubscription::Subscribed
			mqtt_status->setText("MQTT: Connected");
			mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #00ff00; }");
			break;
		}
		case 3: { // QMqttSubscription::UnsubscriptionPending (not yet confirmed by the broker)
			mqtt_status->setText("MQTT: UnsubscriptionPending");
			mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
		case 4: { // QMqttSubscription::Error
			mqtt_status->setText("MQTT: Error");
			mqtt_status->setStyleSheet("QLabel { border-radius: 5px; background-color: #a9a9a9; color: #ff0000; }");
			break;
		}
	}
}

void MainWindow::updateChartSelect(int index) {
	qDebug() << "Sensor chart index changed to " << index;

	/* !===== for testing =====! */
	series[0]->append(series[0]->points().size(), index + 1);
	series[1]->append(series[1]->points().size(), index + 10);
	series[2]->append(series[2]->points().size(), index + 1000);
	/* !======================! */

	this->updateChart();
}

void MainWindow::updateChart() {
	qDebug() << "Updating chart";

	for (int i = 0; i < 3; i++) {
		qreal minX, maxX, minY, maxY;
		for (const QPointF& point : series[i]->points()) {
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

		chart[i]->axes(Qt::Horizontal).back()->setRange(minX, maxX);
		const qreal padding = ceil((maxY - minY) * 0.2);
		chart[i]->axes(Qt::Vertical).back()->setRange(minY - padding, maxY + padding);

		chart[i]->update();
		chartView[i]->update();
	}
}
