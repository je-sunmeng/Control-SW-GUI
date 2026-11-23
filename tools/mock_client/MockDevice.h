#pragma once

#include "model/FriendlyFormation.h"
#include "model/Radar.h"
#include "model/StandaloneFalseTarget.h"
#include "net/FrameDecoder.h"
#include "net/IMessage.h"
#include "net/MessageFactory.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>

#include <optional>

class QTcpSocket;
class ProtocolTable;

// 분석SW / 재밍기법발생기 에뮬레이터 (제어SW 연동 시험용)
//  - 제어SW(서버)에 접속 → Heartbeat 로 식별, 끊기면 자동 재접속
//  - 수신 메시지 해석·출력
//  - 재밍기법발생기: 임무 정보(레이다/편대/허위표적) 저장, MissionStart 이후 레이다별 스캔주기마다
//    MainLobeNotify 송신. 탐지결과 = 거리 내 + 잡음섹터 밖의 편대 허위타겟·고정/이동 허위표적
class MockDevice : public QObject {
    Q_OBJECT
public:
    MockDevice(DeviceCode device, const ProtocolTable& table, QObject* parent = nullptr);
    ~MockDevice() override;

    void connectTo(const QString& host, quint16 port);
    void disconnectFromServer();
    void setDetectionsEnabled(bool on) { m_detections = on; }
    void setReconnect(bool on) { m_reconnect = on; }

    DeviceCode device() const { return m_device; }
    bool isConnected() const;
    bool missionRunning() const { return m_running; }
    int mainLobesSent() const { return m_mainLobesSent; }
    const QVector<Radar>& radars() const { return m_radars; }

    // 테스트용: 실시간 대신 수동으로 경과 시간을 지정해 MainLobe 생성
    void setManualTime(bool on) { m_manualTime = on; }
    void advanceTo(double simTimeSec);

signals:
    void logLine(const QString& line);
    void connectedChanged(bool connected);
    void messageReceived(MessageId id);
    void mainLobeSent(quint32 radarId, int detections);

private:
    void onReadyRead();
    void onMessage(const MessagePtr& msg);
    bool send(MessageId id, const IMessage& msg);
    void onTick();
    QVector<GeoPoint> detectionsFor(const Radar& radar, double t) const;
    void log(const QString& s);

    DeviceCode m_device;
    const ProtocolTable& m_table;
    MessageFactory m_factory;
    QTcpSocket* m_socket;
    FrameDecoder m_decoder;
    QTimer m_tick;
    QTimer m_retry;
    QString m_host;
    quint16 m_port = 0;
    bool m_reconnect = true;
    bool m_detections = true;
    bool m_manualTime = false;

    // 수신 임무 정보
    QVector<Radar> m_radars;
    std::optional<FriendlyFormation> m_formation;
    QVector<StandaloneFalseTarget> m_falseTargets;
    struct Pos {
        GeoPoint pos;
        double heading = 0.0;
        bool valid = false;
    };
    Pos m_formationPos;
    Pos m_jammerPos;

    bool m_running = false;
    QElapsedTimer m_clock;
    QHash<quint32, double> m_nextLobe;
    int m_mainLobesSent = 0;
};
