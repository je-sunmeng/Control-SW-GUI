#pragma once

#include "FrameDecoder.h"
#include "common/Types.h"

#include <QObject>

#include <optional>

class QTcpSocket;

// 접속 클라이언트 1개. 최초 수신 프레임 Opcode 의 Src 로 장치 식별 (TBD-3: 접속 IP 설정 방식은 미사용)
class ClientSession : public QObject {
    Q_OBJECT
public:
    ClientSession(QTcpSocket* socket, quint32 maxPayload, QObject* parent = nullptr);
    ~ClientSession() override;

    std::optional<DeviceCode> device() const { return m_device; }
    QString peer() const { return m_peer; }
    bool write(const QByteArray& frame);
    void close();

signals:
    void identified(ClientSession* session, DeviceCode device);
    void frameReceived(ClientSession* session, const FrameDecoder::Frame& frame);
    void decodeError(ClientSession* session, const QString& error);
    void closed(ClientSession* session);

private:
    void onReadyRead();

    QTcpSocket* m_socket;
    FrameDecoder m_decoder;
    std::optional<DeviceCode> m_device;
    QString m_peer;
    bool m_closed = false;
    int m_reportedErrors = 0;
};
