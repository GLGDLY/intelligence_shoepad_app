#include "infobox.hpp"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

void showInfoBox(const QString& message) {
	QDialog dialog;
	QVBoxLayout layout(&dialog);
	QLabel label(message);
	QPushButton button("OK");
	button.connect(&button, &QPushButton::clicked, &dialog, &QDialog::close);
	layout.addWidget(&label);
	layout.addWidget(&button);
	dialog.setLayout(&layout);
	dialog.exec();
}