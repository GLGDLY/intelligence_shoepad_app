#ifndef _MQTT_APP_HPP
#define _MQTT_APP_HPP

#include "macro_utils.h"

#include <Qtmqtt/QMqttClient>
#include <qobjectdefs.h>

#define ESP_mqtt_state_label_TABLE(X) \
	X(STATUS_OFFLINE)                 \
	X(STATUS_ONLINE)

typedef enum {
	ESP_mqtt_state_label_TABLE(X_EXPAND_ENUM) NUM_OF_ESP_mqtt_state_label,
} esp_mqtt_state_label_t;

class MqttApp : public QObject {
	Q_OBJECT
public:
	explicit MqttApp(QObject* parent = nullptr);
	~MqttApp();

	template <typename Func1, typename Func2>
	void connect_client_signal(Func1 signal,
							   const typename QtPrivate::ContextTypeForFunctor<Func2>::ContextType* context,
							   Func2&& slot) {
		connect(client, signal, context, slot);
	}

Q_SIGNALS:
	void dataReceived(const QByteArray& message, const QMqttTopicName& topic);

private slots:
	void onConnected();
	void onMessage(const QByteArray& message, const QMqttTopicName& topic);

private:
	QMqttClient* client;
	QMqttSubscription* subscription;
};

#endif // _MQTT_APP_HPP
