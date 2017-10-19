/*
 * HeadlessCommunication.cpp
 *
 *  Created on: Oct 9, 2017
 *      Author: misha
 */

#include <src/communication/HeadlessCommunication.hpp>
#include <QHostAddress>

#define PORT 10001

Logger HeadlessCommunication::logger = Logger::getLogger("HeadlessCommunication");

HeadlessCommunication::HeadlessCommunication(QObject* parent) : QObject(parent), m_pSocket(NULL) {}

HeadlessCommunication::~HeadlessCommunication() {
    logger.info("About to die");
    if (m_pSocket != NULL) {
        if (m_pSocket->isOpen()) {
            m_pSocket->close();
        }
        delete m_pSocket;
        m_pSocket = NULL;
    }
}

void HeadlessCommunication::connect() {
    if (m_pSocket == NULL) {
        m_pSocket = new QTcpSocket(this);
    }
    if (!m_pSocket->isOpen()) {
        logger.info("Connect to Last.app UI");
        m_pSocket->connectToHost(QHostAddress::LocalHost, PORT);
        bool res = QObject::connect(m_pSocket, SIGNAL(connected()), this, SLOT(onConnected()));
        Q_ASSERT(res);
        res = QObject::connect(m_pSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        Q_ASSERT(res);
        res = QObject::connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
        Q_ASSERT(true);
        Q_UNUSED(res);
    }
}

void HeadlessCommunication::onConnected() {
    logger.info("Connected to Last.app UI.");
    emit connected();
}

void HeadlessCommunication::onDisconnected() {
    logger.info("Disconnected from Last.app UI. Flushing signals and slots...");
    m_pSocket->close();
    bool res = QObject::disconnect(m_pSocket, SIGNAL(connected()), this, SLOT(onConnected()));
    Q_ASSERT(res);
    res = QObject::disconnect(m_pSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    Q_ASSERT(res);
    res = QObject::disconnect(m_pSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    Q_ASSERT(true);
    Q_UNUSED(res);

    delete m_pSocket;
    m_pSocket = NULL;
    emit closed();
}

void HeadlessCommunication::readyRead() {
    if (m_pSocket->bytesAvailable()) {
        QString command = m_pSocket->readAll();
        emit commandReceived(command);
    }
}

void HeadlessCommunication::send(const QString& command) {
    if (m_pSocket != NULL && m_pSocket->isOpen()) {
        m_pSocket->write(command.toUtf8());
        m_pSocket->flush();
    }
}

