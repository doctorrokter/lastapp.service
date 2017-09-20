/*
 * TrackController.cpp
 *
 *  Created on: Jun 30, 2017
 *      Author: misha
 */

#include "TrackController.hpp"
#include "LastFMCommon.hpp"
#include <QUrl>
#include <QCryptographicHash>
#include <QDebug>
#include "LastFM.hpp"
#include <bb/data/JsonDataAccess>

using namespace bb::data;

namespace bb {
    namespace lastfm {
        namespace controllers {

            Logger TrackController::logger = Logger::getLogger("TrackController");

            TrackController::TrackController(QObject* parent) : QObject(parent) {
                m_pNetwork = new QNetworkAccessManager(this);
            }

            TrackController::~TrackController() {
                m_pNetwork->deleteLater();
            }

            void TrackController::updateNowPlaying(const QString& artist, const QString& track, const QString& album) {
                QNetworkRequest req;

                QUrl url(API_ROOT);
                req.setUrl(url);
                req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

                QString sk = m_settings.value(LAST_FM_KEY).toString();
                QString sig = QString("api_key").append(API_KEY)
                            .append("artist").append(artist.toUtf8())
                            .append("method").append(TRACK_UPDATE_NOW_PLAYING)
                            .append("sk").append(sk)
                            .append("track").append(track.toUtf8())
                            .append(SECRET);
                if (!album.isEmpty()) {
                    sig.prepend(QString("album").append(album));
                }
                QString hash = QCryptographicHash::hash(sig.toAscii(), QCryptographicHash::Md5).toHex();

                QUrl body = LastFM::defaultBody(TRACK_UPDATE_NOW_PLAYING);
                body.addQueryItem("artist", artist.toUtf8());
                body.addQueryItem("track", track.toUtf8());
                body.addQueryItem("sk", sk);
                body.addQueryItem("api_sig", hash);
                if (!album.isEmpty()) {
                    body.addQueryItem("album", album.toUtf8());
                }

                logger.info(url);
                logger.info(body);

                QNetworkReply* reply = m_pNetwork->post(req, body.encodedQuery());
                bool res = QObject::connect(reply, SIGNAL(finished()), this, SLOT(onNowPlayingUpdated()));
                Q_ASSERT(res);
                res = QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
                Q_ASSERT(res);
                Q_UNUSED(res);
            }

            void TrackController::onNowPlayingUpdated() {
                QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

                if (reply->error() == QNetworkReply::NoError) {
                    logger.info(reply->readAll());
                }

                reply->deleteLater();
            }

            void TrackController::scrobble(const QString& artist, const QString& track, const int& timestamp, const QString& album) {
                QNetworkRequest req;

                QUrl url(API_ROOT);
                req.setUrl(url);
                req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

                QString sk = m_settings.value(LAST_FM_KEY).toString();
                QString sig = QString("api_key").append(API_KEY)
                                .append("artist").append(artist.toUtf8())
                                .append("method").append(TRACK_SCROBBLE)
                                .append("sk").append(sk)
                                .append("timestamp").append(QString::number(timestamp))
                                .append("track").append(track.toUtf8())
                                .append(SECRET);
                if (!album.isEmpty()) {
                    sig.prepend(QString("album").append(album));
                }
                QString hash = QCryptographicHash::hash(sig.toAscii(), QCryptographicHash::Md5).toHex();

                QUrl body = LastFM::defaultBody(TRACK_SCROBBLE);
                body.addQueryItem("artist", artist.toUtf8());
                body.addQueryItem("track", track.toUtf8());
                body.addQueryItem("timestamp", QString::number(timestamp));
                body.addQueryItem("sk", sk);
                body.addQueryItem("api_sig", hash);
                if (!album.isEmpty()) {
                    body.addQueryItem("album", album.toUtf8());
                }

                logger.info(url);
                logger.info(body);

                QNetworkReply* reply = m_pNetwork->post(req, body.encodedQuery());
                bool res = QObject::connect(reply, SIGNAL(finished()), this, SLOT(onScrobbled()));
                Q_ASSERT(res);
                res = QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
                Q_ASSERT(res);
                Q_UNUSED(res);
            }

            void TrackController::onScrobbled() {
                QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());

                if (reply->error() == QNetworkReply::NoError) {
                    logger.info(reply->readAll());
                }

                reply->deleteLater();
                emit scrobbled();
            }

            void TrackController::onError(QNetworkReply::NetworkError e) {
                QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
                logger.error(e);
                logger.error(reply->errorString());
                reply->deleteLater();
            }
        }
    }
}
