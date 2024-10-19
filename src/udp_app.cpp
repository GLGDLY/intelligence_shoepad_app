#include "udp_app.hpp"

#include "msgbox_utils.hpp"

#include <QApplication>
#include <QHostAddress>
#include <QThread>


const char connection_search[] = "search";
const char connection_found[] = "found";

UServer::UServer(QObject* parent) : QObject(parent) {
	udpSocket = new QUdpSocket(this);
	connect(udpSocket, &QUdpSocket::readyRead, this, &UServer::readPendingDatagrams);
}

UServer::~UServer() { delete udpSocket; }

void UServer::start() {
	// boardcast to port 1884 to check if any instance already exists in local network, if yes, quit
	QUdpSocket sendSock;
	sendSock.writeDatagram(connection_search, QHostAddress::Broadcast, 1884);
	// wait for 1 second
	QThread::msleep(1000);
	if (sendSock.hasPendingDatagrams()) {
		QByteArray data;
		data.resize(sendSock.pendingDatagramSize());
		QHostAddress sender;
		quint16 port;
		sendSock.readDatagram(data.data(), data.size(), &sender, &port);
		qDebug() << "[UDP] Received datagram: " << data << " from " << sender.toString() << ":" << port;
		if (data.compare(connection_found) == 0) {
			qDebug() << "[UDP] Another instance found, exiting";
			MsgBox box(nullptr, QMessageBox::Icon::Critical, "Error",
					   "Another instance already exists in local network", QMessageBox::Ok, QMessageBox::Ok);
			connect(box.msgBox, &QMessageBox::finished, qApp, &QApplication::quit);
			box.show();
			return;
		}
	}

	qDebug() << "[UDP] Starting server";

	// create server
	if (!udpSocket->bind(QHostAddress::Any, 1884, QUdpSocket::ShareAddress)) {
		qDebug() << "[UDP] Failed to bind socket";
		MsgBox box(nullptr, QMessageBox::Icon::Critical, "Error", "Failed to bind socket", QMessageBox::Ok,
				   QMessageBox::Ok);
		connect(box.msgBox, &QMessageBox::finished, qApp, &QApplication::quit);
		box.show();
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
