#include "msgbox_utils.hpp"

#include <QApplication>
#include <QEventLoop>
#include <QMessageBox>
#include <QTimer>


MsgBox::MsgBox(QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text,
			   QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
	: msgBox(new QMessageBox) {
	msgBox->setParent(parent);
	msgBox->setWindowTitle(title);
	msgBox->setText(text);
	msgBox->setIcon(icon);
	msgBox->setStandardButtons(buttons);
	msgBox->setDefaultButton(defaultButton);

	this->moveToThread(qApp->thread());
}

MsgBox::~MsgBox() { delete msgBox; }

void MsgBox::show() {
	QEventLoop loop;
	QTimer::singleShot(0, this, &MsgBox::onShow);
	connect(msgBox, &QMessageBox::finished, &loop, &QEventLoop::quit);
	loop.exec();
}

void MsgBox::onShow() { msgBox->exec(); }
