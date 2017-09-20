/*
 * LastFM.cpp
 *
 *  Created on: Jun 29, 2017
 *      Author: misha
 */

#include "LastFM.hpp"
#include <QCryptographicHash>
#include <bb/data/JsonDataAccess>
#include <QVariantMap>
#include <QUrl>
#include "LastFMCommon.hpp"

using namespace bb::data;

namespace bb {
    namespace lastfm {

        LastFM::LastFM(QObject* parent) : QObject(parent) {
            m_pTrack = new TrackController(this);
        }

        LastFM::~LastFM() {
            m_pTrack->deleteLater();
        }

        QUrl LastFM::defaultUrl(const QString& method) {
            QUrl url(API_ROOT);
            url.addQueryItem("method", method);
            url.addQueryItem("api_key", API_KEY);
            url.addQueryItem("format", "json");
            return url;
        }

        QUrl LastFM::defaultBody(const QString& method) {
            QUrl body;
            body.addQueryItem("method", method);
            body.addQueryItem("api_key", API_KEY);
            body.addQueryItem("format", "json");
            return body;
        }

        TrackController* LastFM::getTrackController() const { return m_pTrack; }

    }
}
