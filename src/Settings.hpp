/*
 * Settings.hpp
 *
 *  Created on: Oct 19, 2017
 *      Author: misha
 */

#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <QObject>
#include <QVariantMap>
#include "Logger.hpp"

class Settings: public QObject {
    Q_OBJECT
public:
    Settings(QObject* parent = 0);
    virtual ~Settings();

    void setValue(const QString& key, const QVariant& value);
    QVariant value(const QString& key, const QVariant& defVal = "");
    void save();

private:
    static Logger logger;
    QVariantMap m_settings;
};

#endif /* SETTINGS_HPP_ */
