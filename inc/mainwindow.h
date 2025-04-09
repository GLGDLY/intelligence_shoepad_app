#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include "data_container.hpp"
#include "data_recorder.hpp"
#include "graphicsview.hpp"
#include "mqtt_app.hpp"
#include "settings_io.hpp"
#include "worker/chart_worker.hpp"
#include "worker/graphics_worker.hpp"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMutex>
#include <QPixmap>
#include <QQueue>
#include <QRadioButton>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <tuple>


QT_BEGIN_NAMESPACE
namespace Ui {
	class MainWindow;
}
QT_END_NAMESPACE

typedef struct {
	qreal time_sec, X, Y, Z;
} DataPoint;

typedef enum {
	ROT_0,
	ROT_90,
	ROT_180,
	ROT_270,
	NUM_OF_ROTATION,
} Rotation;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private:
	const int data_series_size = 200;

	Ui::MainWindow* ui;

	QComboBox* comboBox;

	QLabel* esp_status_label;
	QHash<QString, qint64> esp_status_map;

	QElapsedTimer elapsed_timer;
	qint64 start_time;

	QChartView* chartView[3];
	QChart* chart[3];
	QLineSeries* series[3];
	QList<QPointF> chart_data[3];
	std::tuple<qreal, qreal> chart_range_y[3];

	QTimer* chart_update_timer;
	QThread* chart_update_thread;
	QQueue<DataPoint> data_queue;
	bool is_data_queue_updated;
	QMutex data_queue_mutex;

	QTimer* graphics_update_timer;
	QThread* graphics_update_thread;
	QHash<QString, std::tuple<qreal, qreal, qreal>> graphics_data;
	QHash<QString, qreal> graphics_data_num;
	QMutex graphics_mutex;

	QPushButton *mqtt_state_btn, *start_stop_btn;
	QMqttClient::ClientState mqtt_state;
	QTimer* mqtt_last_received_timer;

	MqttApp* mqtt;

	QHash<QString, DataContainer*> data_map;

	GraphicsManager* graphicsManager;
	QLineEdit *x_input, *y_input;
	QPushButton *xy_save_button, *sensor_recalibration_button;
	QPushButton* rot_button;
	QRadioButton *xy_left_btn, *xy_right_btn;

	QSet<QString> data_clear_flags;

	QHash<QString, bool> sensor_is_left;
	QHash<QString, std::tuple<int, int>> sensor_pos;
	QHash<QString, Rotation> sensor_rot;

	Settings* settings;

	DataRecorder recorder;

	const qint64 getNowNanoSec() const;
	const qint64 getNowMicroSec() const;

	friend class ChartWorker;
	ChartWorker* chart_worker;

	friend class GraphicsWorker;
	GraphicsWorker* graphics_worker;

private slots:
	void updateMQTTLastReceived();

	void updateMQTTStatus(QMqttClient::ClientState state);
	void updateData(const QByteArray& message, const QMqttTopicName& topic);
	void processData(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z);
	void updateCalEndStatus(const QString esp_id, const QString sensor_id);

	void updateChartSelect(int index);
	void reloadChart();

	void addChartData(time_t timestamp, int16_t X, int16_t Y, int16_t Z);

	// void updateChartData();
	// void updateGraphicsData();

	void xySaveButtonClicked();
	void sensorRecalibrationButtonClicked();
	void rotButtonClicked();

	// replay related
	void mqttStateBtnClicked();
	void startStopBtnClicked();
	void replayFinished();

	void updateEspStatus(const QString esp_id, bool status);

	void clear();
};
#endif // _MAINWINDOW_H
