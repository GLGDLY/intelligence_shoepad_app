#include "mqtt_app.hpp"

#include <QtLogging>
#include <Qtmqtt/QMqttMessage>

MqttApp::MqttApp(QObject* parent) : QObject(parent), client(new QMqttClient(this)) {
	client->setHostname("localhost");
	client->setPort(1883);
	client->setClientId("intelligence_shoepad_app");
	client->setCleanSession(true);
	client->setKeepAlive(60);
	client->setWillTopic("shoepad/data");
	client->setWillMessage("offline");
	client->setWillQoS(0);
	client->setWillRetain(true);

	client->connectToHost();
}

MqttApp::~MqttApp() { delete client; }