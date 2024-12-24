#include "data_recorder.hpp"

#include "infobox.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>


/*
obj = {
	"init_time": 0,

	{key}: [
		[timestamp, T, X, Y, Z],
		[timestamp, T, X, Y, Z],
		...
	],
	...
}
*/

/* DataReplayThread */
DataReplayThread::DataReplayThread() {}

DataReplayThread::~DataReplayThread() {}

void DataReplayThread::run() {
	qDebug() << "Replay thread started";
	while (true) {
		emit processReplay();
		QThread::usleep(1);
	}
}

/* JsonObjIterator */
JsonArrIterator::JsonArrIterator(QString key_, QJsonArray arr_) : key(key_), arr(arr_), index(0) {}

JsonArrIterator::~JsonArrIterator() {}

bool JsonArrIterator::hasNext() { return this->index < this->arr.size(); }

QJsonValue JsonArrIterator::peekNext() { return this->arr.at(this->index); }

void JsonArrIterator::rmNext() { this->index++; }

QJsonValue JsonArrIterator::next() { return this->arr.at(this->index++); }

QString JsonArrIterator::getKey() { return this->key; }

/* DataRecorder */
DataRecorder::DataRecorder() : replay_start_time(), replay_its() {}

DataRecorder::~DataRecorder() {
	for (auto it : this->replay_its) {
		delete it;
	}
	if (this->obj) {
		delete this->obj;
	}
}

RecorderState DataRecorder::getState() const { return this->state; }

bool DataRecorder::startRecording() {
	qDebug() << "start recording called";
	if (state != RecorderStateIdle) {
		return false;
	}
	state = RecorderStateRecording;

	if (this->obj) {
		delete this->obj;
	}
	this->obj = new QJsonObject();
	this->obj->insert("init_time", QDateTime::currentMSecsSinceEpoch());

	qDebug() << "start recording";

	return true;
}

bool DataRecorder::stopRecording() {
	qDebug() << "stop recording called";
	if (state != RecorderStateRecording) {
		return false;
	}
	state = RecorderStateIdle;

	QJsonDocument doc(*this->obj);
	const QString date = QDateTime().currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
	// create ./recording folder
	QDir dir = QDir::current();
	if (!dir.exists("recordings")) {
		dir.mkdir("recordings");
	}
	dir.cd("recordings");
	QFile file(dir.filePath(QString("recording_%1.json").arg(date)));
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Couldn't open recording file";
		delete this->obj;
		this->obj = nullptr;

		showInfoBox("Failed to save recording file");
		return true;
	}
	file.write(doc.toJson());
	file.close();
	delete this->obj;
	this->obj = nullptr;

	showInfoBox("Recording saved");
	return true;
}

void DataRecorder::dataRecord(QString key, qint64 timestamp, int16_t T, int16_t X, int16_t Y, int16_t Z) {
	if (state != RecorderStateRecording) {
		return;
	}
	QJsonArray data = {timestamp, T, X, Y, Z};
	if (!this->obj->contains(key)) {
		this->obj->insert(key, QJsonArray());
	}
	this->obj->value(key).toArray().append(data);

	qDebug() << "Data recorded: " << key << " " << timestamp << " " << T << " " << X << " " << Y << " " << Z;
}

bool DataRecorder::startReplaying(QString path) {
	qDebug() << "start replay called";
	if (state != RecorderStateIdle) {
		return false;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Couldn't open recording file";
		showInfoBox("Failed to open recording file");
		return false;
	}

	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError) {
		qWarning() << "Couldn't parse recording file";
		showInfoBox("Failed to parse recording file");
		return false;
	}

	QJsonObject new_obj = doc.object();
	if (!new_obj.contains("init_time")) {
		qWarning() << "Invalid recording file: no init_time";
		showInfoBox("Invalid recording file: no init_time");
		return false;
	}

	for (auto key : new_obj.keys()) {
		if (key == "init_time") {
			if (!new_obj.value(key).isDouble()) {
				qWarning() << "Invalid recording file: init_time not number";
				showInfoBox("Invalid recording file: init_time not number");
				return false;
			}
			continue;
		}
		if (!new_obj.value(key).isArray()) {
			qWarning() << "Invalid recording file: data not array";
			showInfoBox("Invalid recording file: data not array");
			return false;
		}
		for (auto data : new_obj.value(key).toArray()) {
			if (!data.isArray() || data.toArray().size() != 5) {
				qWarning() << "Invalid recording file: data not array or size not 5";
				showInfoBox("Invalid recording file: data not array or size not 5");
				return false;
			}
			for (auto val : data.toArray()) {
				if (!val.isDouble()) {
					qWarning() << "Invalid recording file: value not number";
					showInfoBox("Invalid recording file: value not number");
					return false;
				}
			}
		}
	}

	// setup replay
	for (auto it : this->replay_its) {
		delete it;
	}
	if (this->obj) {
		delete this->obj;
	}
	this->obj = new QJsonObject(new_obj);
	for (auto key : this->obj->keys()) {
		if (key == "init_time") {
			continue;
		}
		this->replay_its.append(new JsonArrIterator(key, this->obj->value(key).toArray()));
	}

	this->replay_start_time = QDateTime::currentDateTime();
	this->replay_data_start_time = QDateTime::fromMSecsSinceEpoch(this->obj->value("init_time").toInt());
	this->replay_finished_do_once = true;

	if (this->replay_thread) {
		delete this->replay_thread;
	}
	this->replay_thread = new DataReplayThread();
	connect(this->replay_thread, &DataReplayThread::processReplay, this, &DataRecorder::replayData);
	connect(this, &DataRecorder::replayStop, this->replay_thread, &DataReplayThread::quit);
	connect(this->replay_thread, &DataReplayThread::finished, this, &DataRecorder::deleteReplayThread);
	this->replay_thread->start();

	state = RecorderStateReplaying;
	qDebug() << "start replay";
	return true;
}

bool DataRecorder::stopReplaying() {
	qDebug() << "stop replay called";
	if (state != RecorderStateReplaying) {
		return false;
	}
	state = RecorderStateIdle;
	emit replayStop();
	delete this->obj;
	this->obj = nullptr;
	return true;
}

void DataRecorder::replayData() {
	if (state != RecorderStateReplaying) {
		return;
	}

	const qint64 time_elapsed = this->replay_start_time.msecsTo(QDateTime::currentDateTime());
	bool all_end = true;
	for (auto it : this->replay_its) {
		if (it->hasNext()) {
			all_end = false;
			QJsonArray data = it->peekNext().toArray();
			if (this->replay_data_start_time.msecsTo(QDateTime::fromMSecsSinceEpoch(data.at(0).toInt()))
				< time_elapsed) {
				break;
			}
			emit playbackData(it->getKey(), data.at(0).toInt(), data.at(1).toInt(), data.at(2).toInt(),
							  data.at(3).toInt(), data.at(4).toInt());
		}
	}
	if (all_end && this->replay_finished_do_once) {
		emit replayFinished();
		this->replay_finished_do_once = false;
	}
}

void DataRecorder::deleteReplayThread() {
	delete this->replay_thread;
	this->replay_thread = nullptr;
}
