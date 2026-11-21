#pragma once

#include "common/Types.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>

// 레이다 빔: 현재 조향 방위의 메인로브(빔폭 θ, 길이 = 최대탐지거리) + 양옆 사이드로브.
//  - 형상: sinc² 안테나 패턴 G(φ) 를 극좌표 샘플링, r(φ) = Rmax·G(φ)
//  - 로컬 좌표(km, 북쪽 = −y)에서 생성 후 레이다 위치에 두고 setRotation(방위) 로만 갱신
//    (빔폭/거리 변경 시에만 형상 재계산)
//  - 지나간 자리 잔상(afterglow): 스캔 반대 방향으로 감쇠하는 부채꼴
class RadarBeamItem : public QGraphicsItem {
public:
    static constexpr double AfterglowDeg = 24.0;

    RadarBeamItem(const QColor& color, double maxRangeKm, double beamWidthDeg, ScanDirection dir,
                  QGraphicsItem* parent = nullptr);

    void setBeam(double maxRangeKm, double beamWidthDeg, ScanDirection dir);
    void setAzimuth(double deg) { setRotation(deg); }

    // 안테나 패턴 (전력, 정규화 0~1). φ: 빔 중심 기준 각도(deg)
    static double gain(double offsetDeg, double beamWidthDeg);
    static QPainterPath buildLobePath(double maxRangeKm, double beamWidthDeg);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QColor m_color;
    double m_range;
    double m_beamWidth;
    ScanDirection m_dir;
    QPainterPath m_lobe;
};
