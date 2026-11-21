#include "NetworkManager.h"

#include "ClientSession.h"
#include "MessageHeader.h"
#include "Opcode.h"
#include "TcpServer.h"
#include "common/Constants.h"
#include "log/Logger.h"

namespace {
const QString kNet = QStringLiteral("Net");
} // namespace

NetworkManager::NetworkManager(const ProtocolTable& table, Logger& logger, QObject* parent)
    : QObject(parent)
    , m_table(table)
    , m_logger(logger)
    , m_server(new TcpServer(Net::MaxPayloadSize, this))
{
    qRegisterMetaType<MessagePtr>();
    MessageFactory::registerAll(m_factory);
    connect(m_server, &TcpServer::sessionOpened, this, &NetworkManager::onSessionOpened);
    connect(m_server, &TcpServer::sessionClosed, this, &NetworkManager::onSessionClosed);
}

NetworkManager::~NetworkManager()
{
    // 종료 중 시그널 발생 방지
    disconnect(m_server, nullptr, this, nullptr);
}

bool NetworkManager::setEnabled(bool enabled, quint16 port)
{
    if (enabled == isEnabled())
        return true;
    if (!enabled) {
        m_server->close();
        // 세션 closed 시그널로 연결 해제가 통지되지만, 안전하게 남은 매핑 정리
        const auto devices = m_byDevice.keys();
        m_byDevice.clear();
        for (DeviceCode d : devices)
            emit deviceConnectionChanged(d, false);
        m_logger.info(QStringLiteral("네트워크 비활성 (서버 종료)"), kNet);
        emit listeningChanged(false, 0);
        return true;
    }
    QString err;
    if (!m_server->listen(port, QHostAddress::Any, &err)) {
        m_logger.error(QStringLiteral("TCP 서버 시작 실패 (포트 %1): %2").arg(port).arg(err), kNet);
        emit listeningChanged(false, 0);
        return false;
    }
    m_logger.info(QStringLiteral("TCP 서버 대기 중 (포트 %1)").arg(m_server->port()), kNet);
    emit listeningChanged(true, m_server->port());
    return true;
}

bool NetworkManager::isEnabled() const
{
    return m_server->isListening();
}

quint16 NetworkManager::port() const
{
    return m_server->port();
}

bool NetworkManager::isConnected(DeviceCode dev) const
{
    return m_byDevice.contains(dev);
}

bool NetworkManager::send(DeviceCode dst, const IMessage& msg)
{
    if (!isEnabled())
        return false;
    const QString name = toDisplayString(dst);
    const auto opcode = m_table.opcodeFor(msg.id(), DeviceCode::ControlSW, dst);
    if (!opcode) {
        m_logger.error(QStringLiteral("%1 → %2: protocol.json 매핑 없음, 송신 불가").arg(MessageIds::toString(msg.id()), name), kNet);
        return false;
    }
    ClientSession* s = m_byDevice.value(dst);
    if (!s) {
        m_logger.error(QStringLiteral("%1 미연결: %2 송신 불가").arg(name, MessageIds::toString(msg.id())), kNet);
        return false;
    }
    const QByteArray payload = msg.payload();
    const QByteArray frame = MessageHeader::frame(*opcode, payload);
    const bool ok = s->write(frame);
    if (ok)
        m_logger.tx(name, *opcode, quint32(payload.size()), msg.summary(), frame);
    else
        m_logger.error(QStringLiteral("%1 송신 실패: %2").arg(name, MessageIds::toString(msg.id())), kNet);
    return ok;
}

void NetworkManager::onSessionOpened(ClientSession* s)
{
    m_logger.info(QStringLiteral("클라이언트 접속: %1 (장치 식별 대기)").arg(s->peer()), kNet);
    connect(s, &ClientSession::identified, this, &NetworkManager::onIdentified);
    connect(s, &ClientSession::decodeError, this, [this](ClientSession* ss, const QString& e) {
        m_logger.error(QStringLiteral("%1 수신 프레임 오류: %2").arg(ss->peer(), e), kNet);
    });
    connect(s, &ClientSession::frameReceived, this, [this](ClientSession* ss, const FrameDecoder::Frame& f) {
        const quint8 src = Opcode::src(f.header.opcode);
        const QString who = ss->device() ? toDisplayString(*ss->device()) : ss->peer();
        const MessageFactory::DecodeResult res = m_factory.decode(f.header.opcode, f.payload, m_table);
        m_logger.rx(who, f.header.opcode, f.header.dataSize, res.message->summary(), f.raw);
        if (res.known && !res.parsed)
            m_logger.error(QStringLiteral("%1 payload 해석 실패 (%2, %3 bytes)")
                               .arg(who, Opcode::toString(f.header.opcode))
                               .arg(f.payload.size()),
                           kNet);
        if (Opcode::dst(f.header.opcode) != quint8(DeviceCode::ControlSW))
            m_logger.error(QStringLiteral("목적지가 제어SW 가 아닌 프레임 수신 (%1)").arg(Opcode::toString(f.header.opcode)), kNet);
        if (res.parsed)
            emit messageReceived(DeviceCode(src), res.message);
    });
}

void NetworkManager::onIdentified(ClientSession* s, DeviceCode dev)
{
    if (ClientSession* old = m_byDevice.value(dev); old && old != s) {
        m_logger.error(QStringLiteral("%1 중복 접속: 이전 연결(%2) 종료").arg(toDisplayString(dev), old->peer()), kNet);
        m_byDevice.remove(dev);
        old->close();
    }
    m_byDevice.insert(dev, s);
    m_logger.info(QStringLiteral("%1 연결 (%2)").arg(toDisplayString(dev), s->peer()), kNet);
    emit deviceConnectionChanged(dev, true);
}

void NetworkManager::onSessionClosed(ClientSession* s)
{
    if (const auto dev = s->device(); dev && m_byDevice.value(*dev) == s) {
        m_byDevice.remove(*dev);
        m_logger.info(QStringLiteral("%1 연결 해제").arg(toDisplayString(*dev)), kNet);
        emit deviceConnectionChanged(*dev, false);
    } else {
        m_logger.info(QStringLiteral("클라이언트 연결 해제: %1").arg(s->peer()), kNet);
    }
}
