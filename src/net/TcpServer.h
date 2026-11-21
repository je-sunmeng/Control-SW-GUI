#pragma once

#include "common/Types.h"

#include <QHostAddress>
#include <QList>
#include <QObject>

class QTcpServer;
class ClientSession;

// QTcpServer listen(포트 설정), 세션 생성/정리
class TcpServer : public QObject {
    Q_OBJECT
public:
    explicit TcpServer(quint32 maxPayload, QObject* parent = nullptr);
    ~TcpServer() override;

    bool listen(quint16 port, const QHostAddress& address = QHostAddress::Any, QString* error = nullptr);
    void close();   // 서버 + 모든 세션 종료
    bool isListening() const;
    quint16 port() const;   // 실제 대기 포트 (0 지정 시 OS 할당값)

    const QList<ClientSession*>& sessions() const { return m_sessions; }

signals:
    void sessionOpened(ClientSession* session);
    void sessionClosed(ClientSession* session);

private:
    void onNewConnection();

    QTcpServer* m_server;
    quint32 m_maxPayload;
    QList<ClientSession*> m_sessions;
};
