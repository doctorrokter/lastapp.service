/*
 * Settings.cpp
 *
 *  Created on: Oct 19, 2017
 *      Author: misha
 */

#include <src/Settings.hpp>
#include <bb/data/JsonDataAccess.hpp>
#include <QVariantMap>
#include <QFile>
#include <QDir>

#define SETTINGS_PATH "/data/settings.json"

using namespace bb::data;

Logger Settings::logger = Logger::getLogger("Service::Settings");

Settings::Settings(QObject* parent) : QObject(parent) {
    QFile settings(QDir::currentPath() + SETTINGS_PATH);
    if (settings.exists()) {
        if (settings.open(QIODevice::ReadOnly)) {
            JsonDataAccess jda;
            m_settings = jda.loadFromBuffer(settings.readAll()).toMap();
            logger.info("Settings read successfully.");
        } else {
            logger.error("Cannot open settings file to read. " + settings.errorString());
        }
    } else {
        logger.info("Settings file not created yet.");
    }
}

Settings::~Settings() {
    save();
}

void Settings::setValue(const QString& key, const QVariant& value) {
    m_settings[key] = value;
}

QVariant Settings::value(const QString& key, const QVariant& defVal) {
    if (m_settings.contains(key)) {
        return m_settings[key];
    }
    return defVal;
}

void Settings::save() {
    JsonDataAccess jda;
    QFile settings(QDir::currentPath() + SETTINGS_PATH);
    if (settings.open(QIODevice::WriteOnly)) {
        jda.save(m_settings, &settings);
        logger.info("Settings saved successfully");
    } else {
        logger.error("Cannot open settings file to write. " + settings.errorString());
    }
}
