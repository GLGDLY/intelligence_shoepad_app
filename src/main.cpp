#include "mainwindow.h"
#include "network.hpp"

#include <QApplication>
#include <QLocale>
#include <QThread>
#include <QTranslator>
#include <QtLogging>

// disbale clang-format, cuz windows.h need to be included first
// clang-format off
#ifdef _WIN32
	#include <windows.h>
	#include <errhandlingapi.h>
	#include <minwindef.h>
	#include <winnt.h>
	#include <dbghelp.h>
#endif
// clang-format on


QtMessageHandler handler = nullptr;

void log_file_handler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
	QString message = qFormatLogMessage(type, context, msg);
	static FILE* f = fopen("log.txt", "w");
	fprintf(f, "%s\n", qPrintable(message));
	fflush(f);
}

#ifdef _WIN32
LONG crash_handler(PEXCEPTION_POINTERS pExceptionInfo) {
	qCritical() << "Unhandled exception caught";
	qCritical() << "Exception code: " << pExceptionInfo->ExceptionRecord->ExceptionCode;
	qCritical() << "Exception information: " << pExceptionInfo->ExceptionRecord->ExceptionInformation[0] << " "
				<< pExceptionInfo->ExceptionRecord->ExceptionInformation[1];

	// get information on the exception address
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	STACKFRAME64 sf;
	sf.AddrPC.Offset = pExceptionInfo->ContextRecord->Rip;
	sf.AddrStack.Offset = pExceptionInfo->ContextRecord->Rsp;
	sf.AddrFrame.Offset = pExceptionInfo->ContextRecord->Rbp;

	int stack_depth = 0;
	while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &sf, pExceptionInfo->ContextRecord, 0,
					   SymFunctionTableAccess64, SymGetModuleBase64, 0)) {
		if (sf.AddrFrame.Offset == 0 || stack_depth >= 10) {
			break;
		}

		// get function name
		char symbol_buffer[sizeof(IMAGEHLP_SYMBOL64) + 512];
		IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symbol_buffer;
		symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		symbol->MaxNameLength = 512;
		DWORD64 displacement;
		if (SymGetSymFromAddr64(hProcess, sf.AddrPC.Offset, nullptr, symbol)) {
			// get file name and line number
			DWORD displacement;
			IMAGEHLP_LINE64 line;
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, &displacement, &line)) {
				qCritical() << "Stack frame " << stack_depth << ": " << symbol->Name << " at " << line.FileName << ":"
							<< line.LineNumber;
			} else {
				qCritical() << "Stack frame " << stack_depth << ": " << symbol->Name;
			}
		} else {
			qCritical() << "Stack frame " << stack_depth << ": (unknown)";
		}
		stack_depth++;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char* argv[]) {
	handler = qInstallMessageHandler(log_file_handler);

#ifdef _WIN32
	SymInitialize(GetCurrentProcess(), nullptr, TRUE);
	SetUnhandledExceptionFilter(crash_handler);
#endif

	try {
		// Network network;
		// network.test_tf();

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

	return -1;
}
