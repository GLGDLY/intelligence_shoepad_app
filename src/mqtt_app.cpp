#include "mqtt_app.hpp"

#include <QtLogging>
#include <Qtmqtt/QMqttMessage>

MqttApp::MqttApp(QObject* parent) : QObject(parent), client(new QMqttClient(this)) {
	client->setHostname("localhost");
	client->setPort(1883);
	client->setClientId("intelligence_shoepad_app");
	client->setCleanSession(true);
	client->setKeepAlive(60);

	connect(client, &QMqttClient::connected, this, &MqttApp::onConnected);
	connect(client, &QMqttClient::messageReceived, this, &MqttApp::onMessage);

	client->connectToHost();
}

MqttApp::~MqttApp() { delete client; }

void MqttApp::onConnected() {
	qDebug() << "[MQTT] connected";
	if (subscription) {
		qDebug() << "[MQTT] Subscription already exists, deleting existing one";
		try {
			delete subscription;
		} catch (...) {
			qDebug() << "[MQTT] Failed to delete existing subscription";
		}
		// return;
	}
	subscription = client->subscribe(QString("esp/#"), 0);
	if (!subscription) {
		qDebug() << "[MQTT] Failed to subscribe to esp/#";
		return;
	}
	qDebug() << "[MQTT] Subscribed to esp/#";
}

// esp/%s/status
// esp/%s/d/%d
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
				break;
			}
			case STATUS_ONLINE: {
				qDebug() << "[MQTT] Status update: Device online";
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
	} else {
		qDebug() << "[MQTT] Invalid topic level 2: " << topic.levels().at(2);
	}
}
