#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include "data_container.hpp"
#include "data_recorder.hpp"
#include "graphicsview.hpp"
#include "mqtt_app.hpp"
#include "settings_io.hpp"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMutex>
#include <QPixmap>
#include <QQueue>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>


QT_BEGIN_NAMESPACE
namespace Ui {
	class MainWindow;
}
QT_END_NAMESPACE

typedef struct {
	qreal time_sec, X, Y, Z;
} DataPoint;

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
	QLineSeries* series[3];
	QList<QPointF> chart_data[3];
	std::tuple<qreal, qreal> chart_range_y[3];
	QTimer* chart_update_timer;
	QQueue<DataPoint> data_queue;
	QMutex data_queue_mutex;

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
	const qint64 getNowMicroSec() const;

Q_SIGNALS:
	void sig_setArrowPointingToScalar(QString name, qreal sca_x, qreal sca_y);
	void sig_setDefaultSphereColorScalar(QString name, qreal scalar);

private slots:
	void updateMQTTLastReceived();

	void updateMQTTStatus(QMqttClient::ClientState state);
	void updateData(const QByteArray& message, const QMqttTopicName& topic);
	void processData(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z);
	void updateCalEndStatus(const QString esp_id, const QString sensor_id);

	void updateChartSelect(int index);
	void reloadChart();

	void addChartData(time_t timestamp, int16_t X, int16_t Y, int16_t Z);
	void updateChartData();

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
