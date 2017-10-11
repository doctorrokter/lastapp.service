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

#define SCROBBLE_AFTER 60000
#define SCROBBLER_ENABLE "scrobbler.enable"
#define SCROBBLER_DISABLE "scrobbler.disable"

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
        m_pLastFM(new LastFM(this)),
        m_pCommunication(NULL),
        m_online(true),
        m_initialized(false),
        m_scrobblerEnabled(true) {

    QCoreApplication::setOrganizationName("mikhail.chachkouski");
    QCoreApplication::setApplicationName("Last.app");

    m_invokeManager->connect(m_invokeManager, SIGNAL(invoked(const bb::system::InvokeRequest&)), this, SLOT(handleInvoke(const bb::system::InvokeRequest&)));
    m_notify->setType(NotificationType::AllAlertsOff);

    NotificationDefaultApplicationSettings settings;
    settings.setPreview(NotificationPriorityPolicy::Allow);
    settings.apply();

    logger.info("Constructor called");
}

Service::~Service() {
    m_pNetworkConf->deleteLater();
    m_pLastFM->deleteLater();
    m_pNpc->deleteLater();
    m_invokeManager->deleteLater();
    m_notify->deleteLater();
    if (m_pCommunication != NULL) {
        m_pCommunication->deleteLater();
    }
    logger.info("Service DESTROYED!!!");
}

void Service::init() {
    if (!m_initialized) {
        logger.info("Initialize Service signals and slots");
        m_scrobblerEnabled = m_settings.value("scrobbler_enabled", "true").toBool();
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

    QString notify = m_settings.value("notify_now_playing", "").toString();
    if (notify.isEmpty() || notify.compare("true") == 0) {
        m_notify->notify();
    }
}

void Service::triggerNotification() {
    m_timer.singleShot(2000, this, SLOT(onTimeout()));
}

void Service::onTimeout() {
    notify();
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
        switchScrobbler();
    } else if (action.compare("chachkouski.LastappService.START") == 0 || action.compare("bb.action.RESTART") == 0) {
        logger.info("Service started by UI part");
        init();
        establishCommunication();
    } else {
        if (action.compare("bb.action.system.STARTED") == 0) {
            logger.info("Service started OS restart");
            m_initTimer.singleShot(30000, this, SLOT(init()));
        }
        m_notify->setBody("Started in backgroud");
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

void Service::processCommandFromUI(const QString& command) {
    if (command.compare(SCROBBLER_ENABLE) == 0) {
        if (!m_scrobblerEnabled) {
            m_scrobblerEnabled = true;
            switchScrobbler(true);
        }

    } else if (command.compare(SCROBBLER_DISABLE) == 0) {
        if (m_scrobblerEnabled) {
            m_scrobblerEnabled = false;
            switchScrobbler(true);
        }
    }
}

void Service::switchScrobbler(const bool& fromUI) {
    m_settings.setProperty("scrobbler_enabled", m_scrobblerEnabled);
    m_settings.sync();

    if (!fromUI) {
        if (m_pCommunication != NULL) {
            if (m_scrobblerEnabled) {
                m_pCommunication->send(SCROBBLER_ENABLE);
            } else {
                m_pCommunication->send(SCROBBLER_DISABLE);
            }
        }
    }

    m_notify->setType(NotificationType::Default);
    m_notify->setBody(QString("Scrobbler ").append(m_scrobblerEnabled ? "on" : "off"));
    m_notify->notify();
    m_notify->setType(NotificationType::AllAlertsOff);
}

void Service::establishCommunication() {
    if (m_pCommunication == NULL) {
        m_pCommunication = new HeadlessCommunication(this);
        m_pCommunication->connect();
        bool res = QObject::connect(m_pCommunication, SIGNAL(closed()), this, SLOT(closeCommunication()));
        Q_ASSERT(res);
        res = QObject::connect(m_pCommunication, SIGNAL(commandReceived(const QString&)), this, SLOT(processCommandFromUI(const QString&)));
        Q_ASSERT(res);
        Q_UNUSED(res);
    }
}

void Service::closeCommunication() {
    if (m_pCommunication != NULL) {
        bool res = QObject::disconnect(m_pCommunication, SIGNAL(closed()), this, SLOT(closeCommunication()));
        Q_ASSERT(true);
        res = QObject::disconnect(m_pCommunication, SIGNAL(commandReceived(const QString&)), this, SLOT(processCommandFromUI(const QString&)));
        Q_ASSERT(true);
        Q_UNUSED(res);
        m_pCommunication->deleteLater();
    }
}
