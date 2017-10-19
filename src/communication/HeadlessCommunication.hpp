/*
 * HeadlessCommunication.hpp
 *
 *  Created on: Oct 9, 2017
 *      Author: misha
 */

#ifndef HEADLESSCOMMUNICATION_HPP_
#define HEADLESSCOMMUNICATION_HPP_

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "../Logger.hpp"

class HeadlessCommunication: public QObject {
    Q_OBJECT
public:
    HeadlessCommunication(QObject* parent = 0);
    virtual ~HeadlessCommunication();

    void connect();
    void send(const QString& command);

    Q_SIGNALS:
        void connected();
        void commandReceived(const QString& command);
        void closed();

private slots:
    void onConnected();
    void onDisconnected();
    void readyRead();

private:
    static Logger logger;

    QTcpSocket* m_pSocket;
};

#endif /* HEADLESSCOMMUNICATION_HPP_ */
