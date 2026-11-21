#include "FalseTargetGenerator.h"

#include "common/GeoUtils.h"

#include <cmath>
#include <algorithm>
#include <iterator>

double GhostState::opacityAt(double t) const
{
    if (lifetimeSec <= 0)
        return 0.0;
    const double f = (t - bornAt) / lifetimeSec;
    if (f <= 0.0 || f >= 1.0)
        return 0.0;
    if (f < 0.25)
        return f / 0.25;
    if (f > 0.65)
        return (1.0 - f) / 0.35;
    return 1.0;
}

QVector<GeoPoint> FalseTargetGenerator::formationTargets(const GeoPoint& pos, double headingDeg, const JammingConfig& cfg)
{
    QVector<GeoPoint> out;
    out.reserve(cfg.targets.size());
    for (const FalseTargetOffset& o : cfg.targets) {
        if (o.distanceKm <= 0)
            out.append(pos);
        else
            out.append(GeoUtils::destination(pos, GeoUtils::normalizeDeg(headingDeg + o.angleDeg), o.distanceKm));
    }
    return out;
}

FalseTargetGenerator::FalseTargetGenerator(quint32 seed)
{
    reset(seed);
}

void FalseTargetGenerator::reset(quint32 seed)
{
    m_rng.seed(seed ? seed : QRandomGenerator::global()->generate());
    m_nextSpawn.clear();
    m_ghosts.clear();
    m_nextKey = 1;
}

double FalseTargetGenerator::uniform(double a, double b)
{
    return a + (b - a) * m_rng.generateDouble();
}

void FalseTargetGenerator::updateGhosts(double t, const QVector<Radar>& radars, bool enabled)
{
    if (!enabled) {
        m_ghosts.clear();
        m_nextSpawn.clear();
        return;
    }
    // 수명 끝난 고스트 제거
    m_ghosts.erase(std::remove_if(m_ghosts.begin(), m_ghosts.end(),
                                  [t](const GhostState& g) { return t >= g.bornAt + g.lifetimeSec; }),
                   m_ghosts.end());

    for (const Radar& r : radars) {
        auto it = m_nextSpawn.find(r.id);
        if (it == m_nextSpawn.end())
            it = m_nextSpawn.insert(r.id, t + uniform(0.0, MaxSpawnIntervalSec));
        // 시간 점프가 커도 이미 소멸했을 고스트는 만들지 않음
        if (*it < t - MaxLifetimeSec)
            *it = t - MaxLifetimeSec;
        while (*it <= t) {
            GhostState g;
            g.key = m_nextKey++;
            g.radarId = r.id;
            g.bornAt = *it;
            g.lifetimeSec = uniform(MinLifetimeSec, MaxLifetimeSec);
            // 탐지범위 원 안 균등 분포 (면적 기준: r = R·√u)
            const double range = std::max(MinGhostRangeKm, r.maxRangeKm * std::sqrt(m_rng.generateDouble()));
            g.pos = GeoUtils::destination(r.pos, uniform(0.0, 360.0), range);
            if (t < g.bornAt + g.lifetimeSec)
                m_ghosts.append(g);
            *it += uniform(MinSpawnIntervalSec, MaxSpawnIntervalSec);
        }
    }
}
