#include "mqtt_app.hpp"

#include "udp_app.hpp"

#include <QtLogging>
#include <Qtmqtt/QMqttMessage>


MqttApp::MqttApp(QObject* parent) : QObject(parent), client(new QMqttClient(this)) {
	QString mqtt_hostname = udp_server.start();

	client->setCleanSession(false);
	client->setHostname(mqtt_hostname);
	client->setPort(1883);
	client->setClientId("shoepad_app_" + QString::number(QDateTime::currentSecsSinceEpoch()));
	client->setCleanSession(true);
	client->setKeepAlive(60);

	connect(client, &QMqttClient::connected, this, &MqttApp::onConnected);
	connect(client, &QMqttClient::messageReceived, this, &MqttApp::onMessage);
	connect(client, &QMqttClient::disconnected, this, [this]() { qDebug() << "[MQTT] disconnected"; });
	connect(client, &QMqttClient::errorChanged, this,
			[this](QMqttClient::ClientError error) { qDebug() << "[MQTT] error: " << error; });

	client->connectToHost();
}

MqttApp::~MqttApp() { delete client; }

void MqttApp::publish(const QByteArray& message, const QMqttTopicName& topic) { client->publish(topic, message, 1); }

void MqttApp::onConnected() {
	qDebug() << "[MQTT] connected";
	if (subscription) {
		qDebug() << "[MQTT] Subscription already exists, deleting existing one";
		// try {
		// 	delete subscription;
		// } catch (...) {
		// 	qDebug() << "[MQTT] Failed to delete existing subscription";
		// }
		// return;
	} else {
		subscription = client->subscribe(QString("esp/#"), 0);
		if (!subscription) {
			qDebug() << "[MQTT] Failed to subscribe to esp/#";
			return;
		}
		qDebug() << "[MQTT] Subscribed to esp/#";
	}
}

// esp/%s/status
// esp/%s/d/%d
// esp/%s/cal/%d
void MqttApp::onMessage(const QByteArray& message, const QMqttTopicName& topic) {
	qDebug() << "[MQTT] Received message: " << message << " from topic: " << topic.name();
	if (topic.levelCount() < 3) {
		qDebug() << "[MQTT] Invalid topic level count";
		return;
	}

	// ensure topic valid
	if (topic.levels().at(0).compare("esp") != 0) {
		qDebug() << "[MQTT] Invalid topic level 0";
		return;
	}

	// distribute status and data message
	if (topic.levels().at(2).compare("status") == 0) {
		switch (message.at(0) - '0') {
			case STATUS_OFFLINE: {
				qDebug() << "[MQTT] Status update: Device offline";
				emit updateEspStatus(topic.levels().at(1), false);
				break;
			}
			case STATUS_ONLINE: {
				qDebug() << "[MQTT] Status update: Device online";
				emit updateEspStatus(topic.levels().at(1), true);

				// app/timer/{esp_id}/0, start the esp internal ms timer
				this->publish(QByteArray(), QMqttTopicName(QString("app/timer/%1/0").arg(topic.levels().at(1))));
				break;
			}
			default: {
				qDebug() << "[MQTT] Status update: Invalid status";
				break;
			}
		}
		return;
	} else if (topic.levels().at(2).compare("d") == 0) {
		qDebug() << "[MQTT] Data update: " << message;
		emit dataReceived(message, topic);
	} else if (topic.levels().at(2).compare("cal") == 0) {
		qDebug() << "[MQTT] Calibration end: " << message;
		emit calEndReceived(topic.levels().at(1), topic.levels().at(3));
	} else {
		qDebug() << "[MQTT] Invalid topic level 2: " << topic.levels().at(2);
	}
}
