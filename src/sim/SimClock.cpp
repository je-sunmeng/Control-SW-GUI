#include "SimClock.h"

SimClock::SimClock(QObject* parent)
    : QObject(parent)
{
    m_timer.start();
}

void SimClock::setTimeSource(TimeSource source)
{
    accumulate();
    m_source = std::move(source);
    m_lastMs = now();
}

qint64 SimClock::now() const
{
    return m_source ? m_source() : m_timer.elapsed();
}

void SimClock::accumulate()
{
    const qint64 t = now();
    if (m_running)
        m_base += double(t - m_lastMs) * m_speed / 1000.0;
    m_lastMs = t;
}

void SimClock::start()
{
    m_base = 0.0;
    m_lastMs = now();
    m_running = true;
}

void SimClock::pause()
{
    accumulate();
    m_running = false;
}

void SimClock::resume()
{
    if (m_running)
        return;
    m_lastMs = now();
    m_running = true;
}

void SimClock::stop()
{
    pause();
}

void SimClock::reset()
{
    m_running = false;
    m_base = 0.0;
    m_lastMs = now();
}

void SimClock::setSpeedFactor(int factor)
{
    if (!isValidSpeed(factor) || factor == m_speed)
        return;
    accumulate();   // 이전 배속으로 지난 시간 확정 후 변경
    m_speed = factor;
    emit speedFactorChanged(factor);
}

double SimClock::simTimeSec() const
{
    if (!m_running)
        return m_base;
    return m_base + double(now() - m_lastMs) * m_speed / 1000.0;
}
