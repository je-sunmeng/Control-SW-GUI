#include "ExternalDetectionSource.h"

#include <QMutexLocker>

void ExternalDetectionSource::reset()
{
    QMutexLocker lock(&m_mutex);
    m_queue.clear();
}

void ExternalDetectionSource::push(const Detection& d)
{
    QMutexLocker lock(&m_mutex);
    Detection e = d;
    e.origin = DetectionOrigin::External;
    m_queue.append(e);
}

QVector<Detection> ExternalDetectionSource::collect(const SimContext&, double simTime)
{
    QMutexLocker lock(&m_mutex);
    QVector<Detection> out;
    out.swap(m_queue);
    for (Detection& d : out)
        if (d.simTime <= 0)
            d.simTime = simTime;
    return out;
}
