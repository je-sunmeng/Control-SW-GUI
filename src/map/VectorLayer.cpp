#include "VectorLayer.h"

#include "MapProjection.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPolygonF>

#include <algorithm>
#include <utility>

namespace {

struct Builder {
    const MapProjection& proj;
    QVector<QPolygonF> lines;   // 열린 선
    QVector<QPolygonF> rings;   // 닫힌 링 (면)
    QVector<VectorLayer::PointFeature> points;
    int vertices = 0;
    bool ok = true;

    QPointF pt(const QJsonValue& v)
    {
        const QJsonArray a = v.toArray();
        if (a.size() < 2) {
            ok = false;
            return {};
        }
        ++vertices;
        return proj.toScene({a.at(1).toDouble(), a.at(0).toDouble()});
    }

    QPolygonF poly(const QJsonArray& coords)
    {
        QPolygonF p;
        p.reserve(coords.size());
        for (const QJsonValue& c : coords)
            p << pt(c);
        return p;
    }

    void geometry(const QJsonObject& g, const QString& label)
    {
        const QString type = g.value(QLatin1String("type")).toString();
        const QJsonArray c = g.value(QLatin1String("coordinates")).toArray();
        if (type == QLatin1String("Point")) {
            points.append({pt(g.value(QLatin1String("coordinates"))), label});
        } else if (type == QLatin1String("MultiPoint")) {
            for (const QJsonValue& v : c)
                points.append({pt(v), label});
        } else if (type == QLatin1String("LineString")) {
            lines << poly(c);
        } else if (type == QLatin1String("MultiLineString")) {
            for (const QJsonValue& l : c)
                lines << poly(l.toArray());
        } else if (type == QLatin1String("Polygon")) {
            for (const QJsonValue& r : c)
                rings << poly(r.toArray());
        } else if (type == QLatin1String("MultiPolygon")) {
            for (const QJsonValue& p : c)
                for (const QJsonValue& r : p.toArray())
                    rings << poly(r.toArray());
        } else if (type == QLatin1String("GeometryCollection")) {
            for (const QJsonValue& sub : g.value(QLatin1String("geometries")).toArray())
                geometry(sub.toObject(), label);
        } else if (!type.isEmpty()) {
            ok = false;
        }
    }
};

QString labelOf(const QJsonObject& props)
{
    QString name = props.value(QLatin1String("name")).toString();
    const QJsonValue elev = props.value(QLatin1String("elevation"));
    if (!name.isEmpty() && elev.isDouble())
        name += QStringLiteral(" %1").arg(qRound(elev.toDouble()));
    return name;
}

// Douglas-Peucker (반복형)
QPolygonF simplify(const QPolygonF& pts, double tol)
{
    const int n = int(pts.size());
    if (n < 3)
        return pts;
    QVector<bool> keep(n, false);
    keep[0] = keep[n - 1] = true;
    QVector<std::pair<int, int>> stack{{0, n - 1}};
    const double tol2 = tol * tol;
    while (!stack.isEmpty()) {
        const auto [i, j] = stack.takeLast();
        const QPointF a = pts[i], d = pts[j] - pts[i];
        const double L2 = QPointF::dotProduct(d, d);
        double best = -1;
        int idx = -1;
        for (int k = i + 1; k < j; ++k) {
            const QPointF v = pts[k] - a;
            double t = L2 > 0 ? QPointF::dotProduct(v, d) / L2 : 0.0;
            t = qBound(0.0, t, 1.0);
            const QPointF e = v - t * d;
            const double d2 = QPointF::dotProduct(e, e);
            if (d2 > best) {
                best = d2;
                idx = k;
            }
        }
        if (best > tol2) {
            keep[idx] = true;
            stack.append({i, idx});
            stack.append({idx, j});
        }
    }
    QPolygonF out;
    for (int k = 0; k < n; ++k)
        if (keep[k])
            out << pts[k];
    return out;
}

void addTo(QPainterPath& path, const QPolygonF& p, bool closed)
{
    if (p.size() < 2)
        return;
    path.moveTo(p.first());
    for (int i = 1; i < p.size(); ++i)
        path.lineTo(p[i]);
    if (closed)
        path.closeSubpath();
}

} // namespace

VectorLayer::VectorLayer(QString id, QString displayName)
    : m_id(std::move(id))
    , m_displayName(std::move(displayName))
{
}

void VectorLayer::clear()
{
    m_path = {};
    m_coarse = {};
    m_points.clear();
    m_bounds = {};
    m_hasArea = false;
    m_featureCount = m_vertexCount = m_coarseVertexCount = 0;
}

bool VectorLayer::loadFile(const QString& path, const MapProjection& proj, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("파일 열기 실패: %1 (%2)").arg(path, f.errorString());
        return false;
    }
    return loadGeoJson(f.readAll(), proj, error);
}

bool VectorLayer::loadGeoJson(const QByteArray& json, const MapProjection& proj, QString* error)
{
    clear();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("GeoJSON 파싱 오류: %1 (offset %2)").arg(pe.errorString()).arg(pe.offset);
        return false;
    }

    Builder b{proj, {}, {}, {}, 0, true};
    const QJsonObject root = doc.object();
    const QString type = root.value(QLatin1String("type")).toString();
    auto addFeature = [&](const QJsonObject& f) {
        b.geometry(f.value(QLatin1String("geometry")).toObject(),
                   labelOf(f.value(QLatin1String("properties")).toObject()));
        ++m_featureCount;
    };
    if (type == QLatin1String("FeatureCollection")) {
        for (const QJsonValue& f : root.value(QLatin1String("features")).toArray())
            addFeature(f.toObject());
    } else if (type == QLatin1String("Feature")) {
        addFeature(root);
    } else {
        b.geometry(root, {});
        m_featureCount = 1;
    }
    if (!b.ok) {
        if (error)
            *error = QStringLiteral("지원하지 않는 형식 또는 잘못된 좌표");
        clear();
        return false;
    }

    m_path.setFillRule(Qt::OddEvenFill);
    m_coarse.setFillRule(Qt::OddEvenFill);
    for (const QPolygonF& l : std::as_const(b.lines)) {
        addTo(m_path, l, false);
        const QPolygonF s = simplify(l, CoarseToleranceKm);
        addTo(m_coarse, s, false);
        m_coarseVertexCount += int(s.size());
    }
    for (const QPolygonF& r : std::as_const(b.rings)) {
        addTo(m_path, r, true);
        const QPolygonF s = simplify(r, CoarseToleranceKm);
        if (s.size() >= 3) {
            addTo(m_coarse, s, true);
            m_coarseVertexCount += int(s.size());
        }
    }
    m_hasArea = !b.rings.isEmpty();
    m_points = std::move(b.points);
    m_vertexCount = b.vertices;

    // 크기 0 사각형은 QRectF::united 에서 무시되므로 포인트는 min/max 로 직접 누적
    m_bounds = m_path.boundingRect();
    if (!m_points.isEmpty()) {
        double x0 = m_points.first().scenePos.x(), x1 = x0;
        double y0 = m_points.first().scenePos.y(), y1 = y0;
        for (const PointFeature& p : std::as_const(m_points)) {
            x0 = std::min(x0, p.scenePos.x());
            x1 = std::max(x1, p.scenePos.x());
            y0 = std::min(y0, p.scenePos.y());
            y1 = std::max(y1, p.scenePos.y());
        }
        const QRectF pr(QPointF(x0, y0), QPointF(x1, y1));
        m_bounds = m_path.isEmpty() ? pr : m_bounds.united(pr);
    }
    return true;
}

const QPainterPath& VectorLayer::pathForLod(double lod) const
{
    return lod < CoarseLodThreshold ? m_coarse : m_path;
}
