#include "TcpServer.h"

#include "ClientSession.h"

#include <QTcpServer>
#include <QTcpSocket>

TcpServer::TcpServer(quint32 maxPayload, QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_maxPayload(maxPayload)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
    close();
}

bool TcpServer::listen(quint16 port, const QHostAddress& address, QString* error)
{
    if (m_server->isListening())
        return true;
    if (!m_server->listen(address, port)) {
        if (error)
            *error = m_server->errorString();
        return false;
    }
    return true;
}

void TcpServer::close()
{
    m_server->close();
    const QList<ClientSession*> sessions = m_sessions;
    for (ClientSession* s : sessions)
        s->close();   // closed 시그널 → 목록에서 제거
}

bool TcpServer::isListening() const
{
    return m_server->isListening();
}

quint16 TcpServer::port() const
{
    return m_server->serverPort();
}

void TcpServer::onNewConnection()
{
    while (QTcpSocket* socket = m_server->nextPendingConnection()) {
        auto* session = new ClientSession(socket, m_maxPayload, this);
        m_sessions.append(session);
        connect(session, &ClientSession::closed, this, [this](ClientSession* s) {
            if (!m_sessions.removeOne(s))
                return;
            emit sessionClosed(s);
            s->deleteLater();
        });
        emit sessionOpened(session);
    }
}
