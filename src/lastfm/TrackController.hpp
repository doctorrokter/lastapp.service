/*
 * TrackController.hpp
 *
 *  Created on: Jun 30, 2017
 *      Author: misha
 */

#ifndef TRACKCONTROLLER_HPP_
#define TRACKCONTROLLER_HPP_

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include "../Logger.hpp"

namespace bb {
    namespace lastfm {
        namespace controllers {
            class TrackController: public QObject {
                Q_OBJECT
            public:
                TrackController(QObject* parent = 0);
                virtual ~TrackController();

                Q_INVOKABLE void updateNowPlaying(const QString& artist, const QString& track, const QString& album = "");
                Q_INVOKABLE void scrobble(const QString& artist, const QString& track, const int& timestamp, const QString& album = "");

                Q_SIGNALS:
                    void nowPlayingUpdated();
                    void scrobbled();

            private slots:
                void onNowPlayingUpdated();
                void onScrobbled();
                void onError(QNetworkReply::NetworkError e);

            private:
                static Logger logger;

                QSettings m_settings;
                QNetworkAccessManager* m_pNetwork;
            };
        }
    }
}

#endif /* TRACKCONTROLLER_HPP_ */
