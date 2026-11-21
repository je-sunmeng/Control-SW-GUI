#pragma once

#include "IMessage.h"
#include "MessageFactory.h"
#include "ProtocolTable.h"
#include "common/Types.h"

#include <QHash>
#include <QObject>

class Logger;
class TcpServer;
class ClientSession;

// 장치별 송신 API, 수신 디스패치, 연결상태 시그널
//  - 단독모드: setEnabled(false) → 서버 close, 송신 무시
//  - 송수신 모두 Logger 에 Tx/Rx (장치, Opcode, 크기, 요약, hex) 기록
//  - 소켓 I/O 는 메인 스레드 이벤트루프 (데이터량 적음). 시그널/슬롯만 사용하므로 QThread 이동 가능
class NetworkManager : public QObject {
    Q_OBJECT
public:
    NetworkManager(const ProtocolTable& table, Logger& logger, QObject* parent = nullptr);
    ~NetworkManager() override;

    bool setEnabled(bool enabled, quint16 port);
    bool isEnabled() const;
    quint16 port() const;
    bool isConnected(DeviceCode dev) const;

    // dst 로 msg 송신 (Command Code 는 ProtocolTable 에서 조회, DataSize 자동 기입)
    bool send(DeviceCode dst, const IMessage& msg);

    const MessageFactory& factory() const { return m_factory; }

signals:
    void deviceConnectionChanged(DeviceCode dev, bool connected);
    void messageReceived(DeviceCode src, const MessagePtr& message);
    void listeningChanged(bool listening, quint16 port);

private:
    void onSessionOpened(ClientSession* s);
    void onSessionClosed(ClientSession* s);
    void onIdentified(ClientSession* s, DeviceCode dev);

    const ProtocolTable& m_table;
    Logger& m_logger;
    MessageFactory m_factory;
    TcpServer* m_server;
    QHash<DeviceCode, ClientSession*> m_byDevice;
};
