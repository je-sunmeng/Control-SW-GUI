#pragma once

#include <QElapsedTimer>
#include <QObject>

#include <functional>

// 시뮬레이션 시간(sec). 실시간 경과 × 배속(1/2/4/8)을 누적한다.
// 모든 이동/빔 회전은 이 시간으로 계산 → 배속은 Clock 만 영향.
// 테스트를 위해 실시간 원천(ms)을 주입할 수 있다.
class SimClock : public QObject {
    Q_OBJECT
public:
    using TimeSource = std::function<qint64()>;   // 단조 증가 ms

    explicit SimClock(QObject* parent = nullptr);

    void setTimeSource(TimeSource source);

    void start();    // 0부터 시작
    void pause();
    void resume();
    void stop();     // 시간 정지(값 유지)
    void reset();    // 0, 정지

    void setSpeedFactor(int factor);   // 1/2/4/8 (그 외는 무시)
    int speedFactor() const { return m_speed; }
    bool isRunning() const { return m_running; }

    double simTimeSec() const;

    static bool isValidSpeed(int factor) { return factor == 1 || factor == 2 || factor == 4 || factor == 8; }

signals:
    void speedFactorChanged(int factor);

private:
    qint64 now() const;
    void accumulate();   // 현재까지 경과분을 m_base 에 반영

    TimeSource m_source;
    QElapsedTimer m_timer;
    double m_base = 0.0;
    qint64 m_lastMs = 0;
    int m_speed = 1;
    bool m_running = false;
};
