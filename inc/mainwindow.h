#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include "data_container.hpp"
#include "data_recorder.hpp"
#include "graphicsview.hpp"
#include "mqtt_app.hpp"
#include "settings_io.hpp"

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <qdialog.h>
#include <qpixmap.h>


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

	QLabel* esp_status_label;
	QMap<QString, qint64> esp_status_map;

	QElapsedTimer elapsed_timer;
	qint64 start_time;

	QChartView* chartView[3];
	QChart* chart[3];
	QSplineSeries* series[3];
	std::tuple<qreal, qreal> chart_range_y[3];
	QPushButton *mqtt_state_btn, *start_stop_btn;
	QMqttClient::ClientState mqtt_state;
	QTimer* mqtt_last_received_timer;

	MqttApp* mqtt;

	QMap<QString, DataContainer*> data_map;

	GraphicsManager* graphicsManager;
	QLineEdit *x_input, *y_input;
	QPushButton *xy_save_button, *sensor_recalibration_button;

	QSet<QString> data_clear_flags;

	Settings* settings;

	DataRecorder recorder;

	const qint64 getNowNanoSec() const;

private slots:
	void updateMQTTLastReceived();

	void updateMQTTStatus(QMqttClient::ClientState state);
	void updateData(const QByteArray& message, const QMqttTopicName& topic);
	void processData(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z);
	void updateCalEndStatus(const QString esp_id, const QString sensor_id);

	void updateChartSelect(int index);
	void reloadChart();
	void addChartData(time_t timestamp, int16_t X, int16_t Y, int16_t Z);

	void xySaveButtonClicked();
	void sensorRecalibrationButtonClicked();

	// replay related
	void mqttStateBtnClicked();
	void startStopBtnClicked();
	void replayFinished();

	void updateEspStatus(const QString esp_id, bool status);

	void clear();
};
#endif // _MAINWINDOW_H
