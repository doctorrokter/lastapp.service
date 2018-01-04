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

#include "service.hpp"

#include <bb/Application>
#include <bb/platform/Notification>
#include <bb/platform/NotificationDefaultApplicationSettings>
#include <bb/system/InvokeManager>
#include <bb/multimedia/MetaData>
#include <QDateTime>
#include <bb/data/JsonDataAccess>
#include <QFile>
#include <QSettings>

#define LAST_FM_KEY "lastfm_key"
#define SCROBBLE_AFTER 60000
#define SETTINGS_SCROBBLER_KEY "scrobbler.enabled"
#define SETTINGS_NOW_PLAYING_KEY "notify_now_playing"

using namespace bb::platform;
using namespace bb::system;
using namespace bb::multimedia;
using namespace bb::data;

Logger Service::logger = Logger::getLogger("Service");

Service::Service() : QObject(),
        m_notify(new Notification(this)),
        m_invokeManager(new InvokeManager(this)),
        m_pNpc(new NowPlayingController(this)),
        m_pNetworkConf(new QNetworkConfigurationManager(this)),
        m_pLastFM(new LastFM("", this)),
        m_pWatcher(new QFileSystemWatcher(this)),
        m_online(true),
        m_initialized(false) {

    QCoreApplication::setOrganizationName("mikhail.chachkouski");
    QCoreApplication::setApplicationName("Lastapp");

    m_invokeManager->connect(m_invokeManager, SIGNAL(invoked(const bb::system::InvokeRequest&)), this, SLOT(handleInvoke(const bb::system::InvokeRequest&)));
    bool res = QObject::connect(m_pWatcher, SIGNAL(fileChanged(const QString&)), this, SLOT(onFileChanged(const QString&)));
    Q_ASSERT(res);
    Q_UNUSED(res);

    m_notify->setType(NotificationType::AllAlertsOff);
    NotificationDefaultApplicationSettings settings;
    settings.setPreview(NotificationPriorityPolicy::Allow);
    settings.apply();

    QSettings qsettings;
    qsettings.setValue("headless.started", true);
    qsettings.sync();

    m_scrobblerEnabled = qsettings.value(SETTINGS_SCROBBLER_KEY, true).toBool();
    m_notificationsEnabled = qsettings.value(SETTINGS_NOW_PLAYING_KEY, true).toBool();

    QString accessToken = qsettings.value(LAST_FM_KEY, "").toString();
    m_pLastFM->setAccessToken(accessToken);

    m_pWatcher->addPath(qsettings.fileName());

    logger.info("Constructor called");
}

Service::~Service() {
    m_pNetworkConf->deleteLater();
    m_pLastFM->deleteLater();
    m_pNpc->deleteLater();
    m_invokeManager->deleteLater();
    m_notify->deleteLater();
    m_pWatcher->deleteLater();
    logger.info("Service DESTROYED!!!");
}

void Service::init() {
    if (!m_initialized) {
        logger.info("Initialize Service signals and slots");
        m_online = m_pNetworkConf->isOnline();

        bool result = QObject::connect(m_pNpc, SIGNAL(metaDataChanged(QVariantMap)), this, SLOT(nowPlayingChanged(QVariantMap)));
        Q_ASSERT(result);
        result = QObject::connect(m_pNpc, SIGNAL(mediaStateChanged(bb::multimedia::MediaState::Type)), this, SLOT(mediaStateChanged(bb::multimedia::MediaState::Type)));
        Q_ASSERT(result);

        result = QObject::connect(m_pNetworkConf, SIGNAL(onlineStateChanged(bool)), this, SLOT(onOnlineChanged(bool)));
        Q_ASSERT(result);
        Q_UNUSED(result);
        m_initialized = true;
    } else {
        logger.info("Signals and slots already initialized");
    }
}

void Service::notify() {
    Notification::clearEffectsForAll();
    Notification::deleteAllFromInbox();
    m_notify->setTitle("Last.app");

    if (m_notificationsEnabled) {
        m_notify->notify();
    }
}

void Service::triggerNotification() {
    m_timer.singleShot(2000, this, SLOT(onTimeout()));
}

void Service::onTimeout() {
    notify();
}

void Service::onFileChanged(const QString& path) {
    logger.debug("File changed: " + path);
    if (path.contains(".conf")) {
        logger.debug("Settings changed!");
        QSettings qsettings;
        bool sEnabled = qsettings.value(SETTINGS_SCROBBLER_KEY, true).toBool();
        if (sEnabled != m_scrobblerEnabled) {
            m_scrobblerEnabled = sEnabled;
            switchScrobbler();
        }
        m_notificationsEnabled = qsettings.value(SETTINGS_NOW_PLAYING_KEY, true).toBool();
        QString accessToken = qsettings.value(LAST_FM_KEY, "").toString();
        m_pLastFM->setAccessToken(accessToken);
        logger.debug("LastFM key: " + accessToken);
    }
}

void Service::nowPlayingChanged(QVariantMap metadata) {
    if (m_scrobblerEnabled) {
        m_scrobbleTimer.stop();

        m_track.artist = metadata.value(MetaData::Artist, "").toString();
        m_track.name = metadata.value(MetaData::Title, "").toString();
        m_track.album = metadata.value(MetaData::Album, "").toString();
        m_track.duration = metadata.value(MetaData::Duration, 0).toInt();
        m_track.scrobbled = false;
        m_track.timestamp = QDateTime::currentDateTimeUtc().toTime_t();
        m_notify->setBody(
                QString(m_track.artist)
                .append(" - ")
                .append(m_track.name)
                .append(", ")
                .append(m_track.album)
        );
        notify();

        if (m_online) {
            logger.info("Update Now Playing");
            logger.info(m_track.toString());
            m_pLastFM->getTrackController()->updateNowPlaying(m_track.artist, m_track.name, m_track.album);
        } else {
            logger.info("App offline for updating now playing");
        }

        int interval = m_track.duration == 0 ? SCROBBLE_AFTER : m_track.duration / 2;
        m_scrobbleTimer.singleShot(interval, this, SLOT(scrobble()));
    }
}

void Service::scrobble() {
    if (!m_track.scrobbled) {
        if (m_online) {
            doScrobble(m_track);
        } else {
            if (m_scrobblerEnabled) {
                logger.info("App offline for scrobbling");
                QVariantList list = restoreScrobbles();
                list.append(m_track.toMap());
                storeScrobbles(list);
            }
        }
    }
}

void Service::onOnlineChanged(bool online) {
    Q_UNUSED(online);
    logger.info("Online status changed: " + m_pNetworkConf->isOnline());
    m_online = m_pNetworkConf->isOnline();

    if (m_online) {
        QVariantList list = restoreScrobbles();
        foreach(QVariant var, list) {
            QVariantMap map = var.toMap();
            Track track;
            track.fromMap(map);
            doScrobble(track);
        }
    }
}

void Service::mediaStateChanged(bb::multimedia::MediaState::Type state) {
    if (state == bb::multimedia::MediaState::Stopped) {
        logger.info("Stop scrobbling");
        m_scrobbleTimer.stop();
    }
}

void Service::doScrobble(Track& track) {
    if (m_scrobblerEnabled) {
        logger.info("Do Scrobble");
        logger.info(track.toString());
        m_pLastFM->getTrackController()->scrobble(track.artist, track.name, track.timestamp, track.album);
        track.scrobbled = true;
    }
}

void Service::handleInvoke(const bb::system::InvokeRequest& request) {
    QString action = request.action();
    logger.info(action);

    if (action.compare("bb.action.SHORTCUT") == 0) {
        m_scrobblerEnabled = !m_scrobblerEnabled;
        if (!m_scrobblerEnabled) {
            m_scrobbleTimer.stop();
        }
        QSettings qsettings;
        qsettings.setValue(SETTINGS_SCROBBLER_KEY, m_scrobblerEnabled);
        qsettings.sync();
        switchScrobbler();
    } else if (action.compare("chachkouski.LastappService.START") == 0 || action.compare("bb.action.RESTART") == 0) {
        logger.info("Service started by UI part");
        init();
    } else {
        if (action.compare("bb.action.system.STARTED") == 0) {
            logger.info("Service started OS restart");
            m_initTimer.singleShot(30000, this, SLOT(init()));
        }
        m_notify->setBody("Started in background");
        triggerNotification();
    }
}

void Service::storeScrobbles(const QVariantList& scrobbles) {
    QString path = QDir::currentPath() + "/data/scrobbles.json";
    QFile file(path);
    JsonDataAccess jda;
    jda.save(scrobbles, &file);
    logger.info("Scrobbles saved");
    logger.info(scrobbles);
}

QVariantList Service::restoreScrobbles() {
    QVariantList list;
    QString path = QDir::currentPath() + "/data/scrobbles.json";
    QFile file(path);
    if (file.exists()) {
        JsonDataAccess jda;
        list = jda.load(&file).toList();
        file.remove();
    }
    logger.info("Scrobbles restored");
    logger.info(list);
    return list;
}

void Service::switchScrobbler() {
    m_notify->deleteAllFromInbox();
    m_notify->setType(NotificationType::Default);
    m_notify->setBody(QString("Scrobbler ").append(m_scrobblerEnabled ? "on" : "off"));
    m_notify->notify();
    m_notify->setType(NotificationType::AllAlertsOff);
}
