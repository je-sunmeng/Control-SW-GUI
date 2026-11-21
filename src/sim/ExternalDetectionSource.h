#pragma once

#include "IDetectionSource.h"

#include <QMutex>

// 연동모드: 신호발생장치로부터 수신한 MainLobe 탐지결과 메시지를 탐지로 반영.
// MissionController 가 수신 메시지를 push → 다음 틱 collect() 에서 반환 (자체 계산 없음)
class ExternalDetectionSource : public IDetectionSource {
public:
    void reset() override;
    QVector<Detection> collect(const SimContext& ctx, double simTime) override;

    void push(const Detection& d);

private:
    QMutex m_mutex;
    QVector<Detection> m_queue;
};
