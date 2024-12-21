#ifndef _SETTINGS_IO_HPP
#define _SETTINGS_IO_HPP

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>


class Settings {
public:
	Settings();
	~Settings();

	void load();
	void save();

	void set(const QString& key, const QJsonValue& value);
	QJsonValueRef get(const QString& key);
	bool contains(const QString& key);

	QJsonValueRef operator[](const QString& key);

private:
	QJsonDocument* doc;
	QJsonObject* obj;
};

#endif // _SETTINGS_IO_HPP