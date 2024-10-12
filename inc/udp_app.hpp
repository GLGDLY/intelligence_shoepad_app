#ifndef _UDP_APP_HPP
#define _UDP_APP_HPP

#include <QMainWindow>
#include <QObject>
#include <QUdpSocket>


class UServer : public QObject {
	Q_OBJECT

public:
	UServer(QObject* parent = nullptr);
	~UServer();

	void start();
	void stop();

private slots:
	void readPendingDatagrams();

private:
	QUdpSocket* udpSocket;
};


#endif // _UDP_APP_HPP
