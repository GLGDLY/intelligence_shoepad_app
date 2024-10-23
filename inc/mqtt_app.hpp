#ifndef _MQTT_APP_HPP
#define _MQTT_APP_HPP

#include <Qtmqtt/QMqttClient>
#include <qobjectdefs.h>

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
	void messageReceived(const QByteArray& message, const QMqttTopicName& topic);

private slots:
	void onMessage(const QByteArray& message, const QMqttTopicName& topic);

private:
	QMqttClient* client;
};

#endif // _MQTT_APP_HPP
