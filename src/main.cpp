#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QtLogging>

QtMessageHandler handler = nullptr;

void log_file_handler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
	QString message = qFormatLogMessage(type, context, msg);
	static FILE* f = fopen("log.txt", "w");
	fprintf(f, "%s\n", qPrintable(message));
	fflush(f);
}

int main(int argc, char* argv[]) {
	handler = qInstallMessageHandler(log_file_handler);
	try {
		QApplication a(argc, argv);

		QTranslator translator;
		const QStringList uiLanguages = QLocale::system().uiLanguages();
		for (const QString& locale : uiLanguages) {
			const QString baseName = "intelligence_shoepad_app_" + QLocale(locale).name();
			if (translator.load(":/i18n/" + baseName)) {
				a.installTranslator(&translator);
				break;
			}
		}
		MainWindow w;
		w.show();
		return a.exec();
	} catch (const std::exception& e) {
		qCritical() << e.what();
	} catch (...) {
		qCritical() << "Unknown exception caught";
	}
}
