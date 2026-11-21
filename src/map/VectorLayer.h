#pragma once

#include <QColor>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class MapProjection;

struct LayerStyle {
    QColor stroke;                     // invalid → 외곽선 없음
    qreal width = 1.0;                 // 화면 픽셀 (cosmetic pen)
    Qt::PenStyle penStyle = Qt::SolidLine;
    QColor fill;                       // invalid → 채움 없음 (면 레이어만 의미)
    QColor labelColor;                 // 포인트 라벨 색상
    int labelPointSize = 9;
};

// GeoJSON(Point/LineString/Polygon/Multi*/GeometryCollection) → Scene 좌표 QPainterPath 캐시.
// 축소 시 사용할 단순화(coarse) 경로도 함께 생성한다.
class VectorLayer {
public:
    struct PointFeature {
        QPointF scenePos;
        QString label;
    };

    VectorLayer(QString id, QString displayName);

    bool loadGeoJson(const QByteArray& json, const MapProjection& proj, QString* error = nullptr);
    bool loadFile(const QString& path, const MapProjection& proj, QString* error = nullptr);

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }

    LayerStyle style() const { return m_style; }
    void setStyle(const LayerStyle& s) { m_style = s; }
    qreal zValue() const { return m_z; }
    void setZValue(qreal z) { m_z = z; }

    // 화면 배율(px/km)이 minLod 미만이면 그리지 않음 (시군구 등 확대 시에만 표시)
    double minLod() const { return m_minLod; }
    void setMinLod(double lod) { m_minLod = lod; }
    // 포인트 라벨 표시 최소 배율
    double labelMinLod() const { return m_labelMinLod; }
    void setLabelMinLod(double lod) { m_labelMinLod = lod; }

    // lod(px/km)에 맞는 경로 (CoarseLodThreshold 미만이면 단순화 경로)
    const QPainterPath& pathForLod(double lod) const;
    const QPainterPath& path() const { return m_path; }
    const QPainterPath& coarsePath() const { return m_coarse; }
    bool hasArea() const { return m_hasArea; }
    const QVector<PointFeature>& points() const { return m_points; }

    QRectF boundingRect() const { return m_bounds; }
    int featureCount() const { return m_featureCount; }
    int vertexCount() const { return m_vertexCount; }
    int coarseVertexCount() const { return m_coarseVertexCount; }

    static constexpr double CoarseLodThreshold = 0.6;   // px/km
    static constexpr double CoarseToleranceKm = 1.2;    // 단순화 허용오차 (scene 단위)

private:
    void clear();

    QString m_id;
    QString m_displayName;
    LayerStyle m_style;
    qreal m_z = 0;
    double m_minLod = 0;
    double m_labelMinLod = 0;

    QPainterPath m_path;
    QPainterPath m_coarse;
    bool m_hasArea = false;
    QVector<PointFeature> m_points;
    QRectF m_bounds;
    int m_featureCount = 0;
    int m_vertexCount = 0;
    int m_coarseVertexCount = 0;
};
