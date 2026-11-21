#include "ClientSession.h"

#include "Opcode.h"

#include <QTcpSocket>

ClientSession::ClientSession(QTcpSocket* socket, quint32 maxPayload, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
    , m_decoder(maxPayload)
{
    // IPv4-mapped IPv6 (::ffff:a.b.c.d) 는 IPv4 로 표기
    QHostAddress addr = socket->peerAddress();
    bool isV4 = false;
    const quint32 v4 = addr.toIPv4Address(&isV4);
    if (isV4)
        addr = QHostAddress(v4);
    m_peer = QStringLiteral("%1:%2").arg(addr.toString()).arg(socket->peerPort());
    m_socket->setParent(this);
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        if (m_closed)
            return;
        m_closed = true;
        emit closed(this);
    });
}

ClientSession::~ClientSession() = default;

bool ClientSession::write(const QByteArray& frame)
{
    if (m_closed || m_socket->state() != QAbstractSocket::ConnectedState)
        return false;
    return m_socket->write(frame) == frame.size();
}

void ClientSession::close()
{
    if (m_closed)
        return;
    m_closed = true;
    m_socket->disconnectFromHost();
    emit closed(this);
}

void ClientSession::onReadyRead()
{
    const QVector<FrameDecoder::Frame> frames = m_decoder.feed(m_socket->readAll());
    if (m_decoder.errorCount() > m_reportedErrors) {
        m_reportedErrors = m_decoder.errorCount();
        emit decodeError(this, m_decoder.lastError());
    }
    for (const FrameDecoder::Frame& f : frames) {
        if (!m_device) {
            const quint8 src = Opcode::src(f.header.opcode);
            if (src == quint8(DeviceCode::AnalysisSW) || src == quint8(DeviceCode::SignalGen)) {
                m_device = DeviceCode(src);
                emit identified(this, *m_device);
            }
        }
        emit frameReceived(this, f);
    }
}
