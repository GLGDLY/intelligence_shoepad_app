#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mqtt_app.hpp"

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>


QT_BEGIN_NAMESPACE
namespace Ui {
	class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private:
	Ui::MainWindow* ui;
	QComboBox* comboBox;
	QChartView* chartView[3];
	QChart* chart[3];
	QSplineSeries* series[3];
	QLabel* mqtt_status;
	MqttApp* mqtt;

private slots:
	void updateMQTTStatus(QMqttClient::ClientState state);
	void updateData(const QByteArray& message, const QMqttTopicName& topic);

	void updateChartSelect(int index);
	void updateChart();
};
#endif // MAINWINDOW_H
