#include "udp_app.hpp"

#include <QApplication>
#include <QHostAddress>


const char connection_search[] = "search";
const char connection_found[] = "found";

UServer::UServer(QObject* parent) : QObject(parent) {
	udpSocket = new QUdpSocket(this);
	connect(udpSocket, &QUdpSocket::readyRead, this, &UServer::readPendingDatagrams);
}

UServer::~UServer() { delete udpSocket; }

void UServer::start() {
	if (!udpSocket->bind(QHostAddress::Any, 1884)) {
		qDebug() << "[UDP] Failed to bind socket";
		qApp->exit(-1);
	}
}

void UServer::stop() { udpSocket->close(); }

void UServer::readPendingDatagrams() {
	while (udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(udpSocket->pendingDatagramSize());

		QHostAddress sender;
		quint16 port;
		udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &port);

		// process
		qDebug() << "[UDP] Received datagram: " << datagram << " from " << sender.toString() << ":" << port;
		if (datagram.compare(connection_search) == 0) {
			qDebug() << "[UDP] Connection search received";
			udpSocket->writeDatagram(connection_found, sender, port);
		} else {
			qDebug() << "[UDP] Unknown datagram";
		}
	}
}
