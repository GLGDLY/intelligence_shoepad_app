#include "settings_io.hpp"

#include <QFile>

Settings::Settings() : doc(new QJsonDocument()), obj(new QJsonObject()) { this->load(); }

Settings::~Settings() {
	this->save();
	delete this->doc;
	delete this->obj;
}

void Settings::load() {
	QFile file("settings.json");
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Couldn't open settings.json";
		return;
	}

	QJsonParseError error;
	*doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError) {
		qWarning() << "Couldn't parse settings.json";
		return;
	}

	*obj = doc->object();
}

void Settings::save() {
	QFile file("settings.json");
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Couldn't open settings.json";
		return;
	}

	*doc = QJsonDocument(*obj);
	file.write(doc->toJson());
}

void Settings::set(const QString& key, const QJsonValue& value) { this->obj->insert(key, value); }

QJsonValueRef Settings::get(const QString& key) { return (*this->obj)[key]; }

bool Settings::contains(const QString& key) { return this->obj->contains(key); }

QJsonValueRef Settings::operator[](const QString& key) { return (*this->obj)[key]; }
