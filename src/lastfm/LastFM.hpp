/*
 * LastFM.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: misha
 */

#ifndef LASTFM_HPP_
#define LASTFM_HPP_

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "TrackController.hpp"
#include <QUrl>

using namespace bb::lastfm::controllers;

namespace bb {
    namespace lastfm {

        class LastFM: public QObject {
            Q_OBJECT
        public:
            LastFM(QObject* parent = 0);
            virtual ~LastFM();

            static QUrl defaultUrl(const QString& method);
            static QUrl defaultBody(const QString& method);

            TrackController* getTrackController() const;

        private:
            TrackController* m_pTrack;
        };
    }
}

#endif /* LASTFM_HPP_ */
