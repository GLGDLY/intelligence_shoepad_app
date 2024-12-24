#ifndef _DATA_RECORDER_HPP
#define _DATA_RECORDER_HPP

#include "macro_utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QThread>
#include <Qtmqtt/QMqttClient>
#include <qdatetime.h>
#include <qjsonarray.h>


#define RECORDER_STATE_TABLE(X) \
	X(RecorderStateIdle)        \
	X(RecorderStateRecording)   \
	X(RecorderStateReplaying)

typedef enum {
	RECORDER_STATE_TABLE(X_EXPAND_ENUM) NUM_OF_RECORDER_STATE,
} RecorderState;

class DataReplayThread : public QThread {
	Q_OBJECT
public:
	DataReplayThread();
	~DataReplayThread();

	void run() override;

Q_SIGNALS:
	void processReplay();
};

class JsonArrIterator {
public:
	JsonArrIterator(QString key_, QJsonArray arr_);
	~JsonArrIterator();

	bool hasNext();
	QJsonValue peekNext();
	void rmNext();
	QJsonValue next();

	QString getKey();

private:
	QString key;
	QJsonArray arr;
	int index;
};


class DataRecorder : public QObject {
	Q_OBJECT
public:
	DataRecorder();
	~DataRecorder();

	RecorderState getState() const;

	bool startRecording();
	bool stopRecording();

	void dataRecord(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z);

	bool startReplaying(QString path);
	bool stopReplaying();

Q_SIGNALS:
	void playbackData(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z);
	void replayStop();
	void replayFinished();

private slots:
	void replayData();
	void deleteReplayThread();

private:
	RecorderState state = RecorderStateIdle;
	QJsonObject* obj = nullptr;
	DataReplayThread* replay_thread = nullptr;

	// for replay control
	QDateTime replay_start_time, replay_data_start_time;
	QList<JsonArrIterator*> replay_its;
	bool replay_finished_do_once = false;
};

#endif // _DATA_RECORDER_HPP
