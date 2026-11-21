#include "MockDevice.h"

#include "common/Constants.h"
#include "common/GeoUtils.h"
#include "net/MessageHeader.h"
#include "net/Opcode.h"
#include "net/ProtocolTable.h"
#include "net/messages/FalseTargetInfoMsg.h"
#include "net/messages/FormationInfoMsg.h"
#include "net/messages/HeartbeatMsg.h"
#include "net/messages/MainLobeNotifyMsg.h"
#include "net/messages/MissionStartMsg.h"
#include "net/messages/PositionMsg.h"
#include "net/messages/RadarInfoMsg.h"
#include "sim/FalseTargetGenerator.h"
#include "sim/LocalDetectionSource.h"
#include "sim/RouteKinematics.h"

#include <QDateTime>
#include <QTcpSocket>
#include <algorithm>
#include <iterator>

MockDevice::MockDevice(DeviceCode device, const ProtocolTable& table, QObject* parent)
    : QObject(parent)
    , m_device(device)
    , m_table(table)
    , m_socket(new QTcpSocket(this))
    , m_decoder(Net::MaxPayloadSize)
{
    MessageFactory::registerAll(m_factory);
    connect(m_socket, &QTcpSocket::connected, this, [this] {
        m_decoder.reset();
        log(QStringLiteral("접속 성공 %1:%2").arg(m_host).arg(m_port));
        HeartbeatMsg hb;
        hb.deviceCode = quint8(m_device);
        send(MessageId::Heartbeat, hb);   // 장치 식별
        emit connectedChanged(true);
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        log(QStringLiteral("연결 끊김"));
        m_running = false;
        emit connectedChanged(false);
        if (m_reconnect)
            m_retry.start();
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_socket->state() == QAbstractSocket::UnconnectedState && m_reconnect && !m_retry.isActive())
            m_retry.start();
    });
    connect(m_socket, &QTcpSocket::readyRead, this, &MockDevice::onReadyRead);

    m_retry.setInterval(2000);
    m_retry.setSingleShot(true);
    connect(&m_retry, &QTimer::timeout, this, [this] {
        if (m_socket->state() == QAbstractSocket::UnconnectedState)
            m_socket->connectToHost(m_host, m_port);
    });
    m_tick.setInterval(50);
    connect(&m_tick, &QTimer::timeout, this, &MockDevice::onTick);
}

MockDevice::~MockDevice()
{
    m_reconnect = false;
    m_socket->abort();
}

void MockDevice::log(const QString& s)
{
    emit logLine(QStringLiteral("%1 [%2] %3")
                     .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")), toDisplayString(m_device), s));
}

void MockDevice::connectTo(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_socket->connectToHost(host, port);
}

void MockDevice::disconnectFromServer()
{
    m_reconnect = false;
    m_retry.stop();
    m_socket->disconnectFromHost();
}

bool MockDevice::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool MockDevice::send(MessageId id, const IMessage& msg)
{
    const auto op = m_table.opcodeFor(id, m_device, DeviceCode::ControlSW);
    if (!op) {
        log(QStringLiteral("protocol.json 에 %1 매핑 없음").arg(MessageIds::toString(id)));
        return false;
    }
    const QByteArray payload = msg.payload();
    m_socket->write(MessageHeader::frame(*op, payload));
    log(QStringLiteral("TX %1 %2B %3").arg(Opcode::toString(*op)).arg(payload.size()).arg(msg.summary()));
    return true;
}

void MockDevice::onReadyRead()
{
    for (const FrameDecoder::Frame& f : m_decoder.feed(m_socket->readAll())) {
        const auto res = m_factory.decode(f.header.opcode, f.payload, m_table);
        log(QStringLiteral("RX %1 %2B %3").arg(Opcode::toString(f.header.opcode)).arg(f.header.dataSize).arg(res.message->summary()));
        if (res.parsed)
            onMessage(res.message);
    }
}

void MockDevice::onMessage(const MessagePtr& msg)
{
    emit messageReceived(msg->id());
    switch (msg->id()) {
    case MessageId::RadarInfo:
        m_radars = static_cast<RadarInfoMsg*>(msg.data())->radars;
        break;
    case MessageId::FormationInfo:
        m_formation = static_cast<FormationInfoMsg*>(msg.data())->formation;
        break;
    case MessageId::FalseTargetInfo:
        m_falseTargets = static_cast<FalseTargetInfoMsg*>(msg.data())->targets;
        break;
    case MessageId::MissionStart: {
        const auto* s = static_cast<MissionStartMsg*>(msg.data());
        m_running = true;
        m_clock.start();
        m_nextLobe.clear();
        m_formationPos = {};
        m_jammerPos = {};
        if (s->withStartPositions) {
            m_formationPos = {s->formationStart, 0.0, m_formation.has_value()};
            m_jammerPos = {s->jammerStart, 0.0, true};
        }
        // 레이다별 첫 MainLobe: 0.5 s 후부터 스캔주기마다 (레이다 간 위상차를 두어 겹치지 않게)
        for (int i = 0; i < m_radars.size(); ++i)
            m_nextLobe.insert(m_radars[i].id, 0.5 + 0.7 * i);
        if (m_device == DeviceCode::SignalGen && !m_manualTime)
            m_tick.start();
        log(QStringLiteral("임무 시작 — 레이다 %1기").arg(m_radars.size()));
        break;
    }
    case MessageId::MissionStop:
        m_running = false;
        m_tick.stop();
        log(QStringLiteral("임무 종료 (MainLobe %1회 송신)").arg(m_mainLobesSent));
        break;
    case MessageId::JammerPosition: {
        const auto* p = static_cast<JammerPositionMsg*>(msg.data());
        m_jammerPos = {p->pos, p->headingDeg, true};
        break;
    }
    case MessageId::FormationPosition: {
        const auto* p = static_cast<FormationPositionMsg*>(msg.data());
        m_formationPos = {p->pos, p->headingDeg, true};
        break;
    }
    default:
        break;
    }
}

void MockDevice::onTick()
{
    advanceTo(m_clock.elapsed() / 1000.0);
}

void MockDevice::advanceTo(double t)
{
    if (!m_running || m_device != DeviceCode::SignalGen || !isConnected())
        return;
    for (const Radar& r : std::as_const(m_radars)) {
        double& next = m_nextLobe[r.id];
        if (t < next)
            continue;
        MainLobeNotifyMsg ml;
        ml.radarId = r.id;
        ml.searchTime = next;
        if (m_detections)
            ml.detections = detectionsFor(r, t);
        if (send(MessageId::MainLobeNotify, ml)) {
            ++m_mainLobesSent;
            emit mainLobeSent(r.id, int(ml.detections.size()));
        }
        next += std::max(0.5, r.scanPeriodSec);
        if (next <= t)
            next = t + std::max(0.5, r.scanPeriodSec);
    }
}

QVector<GeoPoint> MockDevice::detectionsFor(const Radar& radar, double t) const
{
    QVector<GeoPoint> candidates;
    if (m_formation && m_formationPos.valid)
        candidates += FalseTargetGenerator::formationTargets(m_formationPos.pos, m_formationPos.heading, m_formation->jamming);
    QVector<NoiseSector> noise;
    for (const StandaloneFalseTarget& ft : m_falseTargets) {
        switch (ft.ftType) {
        case FalseTargetType::Fixed:
            candidates.append(ft.pos);
            break;
        case FalseTargetType::Moving: {
            const KinematicState k = RouteKinematics::stateAt(ft.route, t);
            if (k.valid)
                candidates.append(k.pos);
            break;
        }
        case FalseTargetType::Noise:
            noise.append({radar.id, ft.id, GeoUtils::bearingDeg(radar.pos, ft.pos), ft.detectAngleDeg});
            break;
        case FalseTargetType::Random:
            break;
        }
    }
    QVector<GeoPoint> out;
    for (const GeoPoint& p : std::as_const(candidates)) {
        if (GeoUtils::distanceKm(radar.pos, p) > radar.maxRangeKm)
            continue;
        const double brg = GeoUtils::bearingDeg(radar.pos, p);
        const bool jammed = std::any_of(noise.cbegin(), noise.cend(),
                                        [brg](const NoiseSector& s) { return LocalDetectionSource::inNoiseSector(brg, s); });
        if (!jammed)
            out.append(p);
    }
    return out;
}
