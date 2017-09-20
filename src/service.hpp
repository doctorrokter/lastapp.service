/*
 * Copyright (c) 2013-2015 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SERVICE_H_
#define SERVICE_H_

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QSettings>
#include <bb/multimedia/NowPlayingController>
#include <bb/multimedia/MediaState>
#include <QNetworkConfigurationManager>
#include <QTimer>
#include "lastfm/LastFM.hpp"
#include "Logger.hpp"

namespace bb {
    class Application;
    namespace platform {
        class Notification;
    }
    namespace system {
        class InvokeManager;
        class InvokeRequest;
    }
    namespace multimedia {
        class NowPlayingController;
        class MetaData;
    }
}

using namespace bb::platform;
using namespace bb::system;
using namespace bb::multimedia;
using namespace bb::lastfm;

struct Track {
    QString name;
    QString artist;
    QString album;
    int duration;
    bool scrobbled;
    int timestamp;

    QVariantMap toMap() {
        QVariantMap map;
        map["artist"] = artist;
        map["name"] = name;
        map["album"] = album;
        map["timestamp"] = timestamp;
        map["duration"] = duration;
        map["scrobbled"] = scrobbled;
        return map;
    }

    void fromMap(const QVariantMap& map) {
        artist = map.value("artist", "").toString();
        name = map.value("name", "").toString();
        album = map.value("album", "").toString();
        timestamp = map.value("timestamp", 0).toInt();
        duration = map.value("duration", 0).toInt();
        scrobbled = map.value("scrobbled", false).toBool();
    }

    QString toString() {
        return QString("{artist: ").append(artist).append(", ")
                .append("name: ").append(name).append(", ")
                .append("album: ").append(album).append(", ")
                .append("timestamp: ").append(QString::number(timestamp)).append(", ")
                .append("duration: ").append(QString::number(duration)).append(", ")
                .append("scrobbled: ").append(scrobbled).append("}");
    }
};

class Service: public QObject {
    Q_OBJECT
public:
    Service();
    virtual ~Service();

private slots:
    void init();
    void handleInvoke(const bb::system::InvokeRequest& request);
    void nowPlayingChanged(QVariantMap metadata);
    void scrobble();
    void onOnlineChanged(bool online);
    void mediaStateChanged(bb::multimedia::MediaState::Type state);
    void onTimeout();

private:
    void notify();
    void triggerNotification();
    void doScrobble(Track& track);
    void storeScrobbles(const QVariantList& scrobbles);
    QVariantList restoreScrobbles();

    Notification* m_notify;
    InvokeManager* m_invokeManager;
    NowPlayingController* m_pNpc;
    QNetworkConfigurationManager* m_pNetworkConf;
    LastFM* m_pLastFM;

    Track m_track;
    bool m_online;
    QSettings m_settings;
    QTimer m_timer;
    QTimer m_scrobbleTimer;
    QTimer m_initTimer;
    bool m_initialized;

    static Logger logger;
};

#endif /* SERVICE_H_ */
