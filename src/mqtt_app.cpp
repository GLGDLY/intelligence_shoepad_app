#include "mqtt_app.hpp"

#include <QtLogging>
#include <Qtmqtt/QMqttMessage>

MqttApp::MqttApp(QObject* parent) : QObject(parent), client(new QMqttClient(this)) {
	client->setHostname("localhost");
	client->setPort(1883);
	client->setClientId("intelligence_shoepad_app");
	client->setCleanSession(true);
	client->setKeepAlive(60);
	client->setWillTopic("esp/#");
	client->setWillMessage("offline");
	client->setWillQoS(0);
	client->setWillRetain(true);

	connect(client, &QMqttClient::messageReceived, this, &MqttApp::onMessage);

	client->connectToHost();
}

MqttApp::~MqttApp() { delete client; }

// esp/%s/status
// esp/%s/d/%d
void MqttApp::onMessage(const QByteArray& message, const QMqttTopicName& topic) {
	qDebug() << "Received message: " << message << " from topic: " << topic.name();
	if (topic.levelCount() < 3) {
		qDebug() << "Invalid topic level count";
		return;
	}

	// ensure topic valid
	if (topic.levels().at(0).compare("esp") != 0) {
		qDebug() << "Invalid topic level 0";
		return;
	}

	// distribute status and data message
	if (topic.levels().at(2).compare("status") == 0) {
		switch (message.at(0) - '0') {
			case STATUS_OFFLINE: {
				qDebug() << "Status update: Device offline";
				break;
			}
			case STATUS_ONLINE: {
				qDebug() << "Status update: Device online";
				break;
			}
			default: {
				qDebug() << "Status update: Invalid status";
				break;
			}
		}
		return;
	} else if (topic.levels().at(2).compare("d") == 0) {
		qDebug() << "Data update: " << message;
		emit dataReceived(message, topic);
	} else {
		qDebug() << "Invalid topic level 2: " << topic.levels().at(2);
	}
}
