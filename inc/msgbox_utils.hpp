#ifndef _MSGBOX_UTILS_HPP
#define _MSGBOX_UTILS_HPP

#include <QMessageBox>

class MsgBox : public QObject {
	Q_OBJECT

public:
	MsgBox(QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text,
		   QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
	~MsgBox();

	void show();

	QMessageBox* msgBox;

private slots:
	void onShow();
};

#endif // _MSGBOX_UTILS_HPP
