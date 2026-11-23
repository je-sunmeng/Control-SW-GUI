#include "MapLayerManager.h"

#include "MapProjection.h"
#include "VectorLayer.h"
#include "log/Logger.h"

#include <QSettings>
#include <algorithm>
#include <iterator>

namespace {

struct LayerDef {
    const char* id;          // = 리소스 파일명 (:/map/<id>.geojson)
    const char* name;
    qreal z;
    LayerStyle style;
    double minLod;
    double labelMinLod;
    bool visible;
};

// 표시 순서(z) 및 다크 테마 스타일 — 위경도선(GraticuleZ)은 육지 채움 위, 선 레이어 아래
const LayerDef kDefaults[] = {
    {"countries",       "주변국 육지",     0,  {QColor(0x3a, 0x41, 0x4b), 0.6, Qt::SolidLine, QColor(0x22, 0x27, 0x2e), {}, 9}, 0, 0, true},
    {"kr_land",         "한반도 육지",     1,  {{}, 1.0, Qt::SolidLine, QColor(0x2b, 0x33, 0x3d), {}, 9},                     0, 0, true},
    {"rivers",          "하천",            4,  {QColor(0x2f, 0x5f, 0x8a), 1.0, Qt::SolidLine, {}, {}, 9},                      0, 0, true},
    {"coastline_world", "주변국 해안선",   5,  {QColor(0x5a, 0x62, 0x70), 1.0, Qt::SolidLine, {}, {}, 9},                      0, 0, true},
    {"borders",         "국경",            6,  {QColor(0x7a, 0x82, 0x90), 1.0, Qt::DashLine, {}, {}, 9},                       0, 0, true},
    {"kr_admin1",       "행정구역(시/도)", 7,  {QColor(0x55, 0x63, 0x75), 0.8, Qt::DashLine, {}, {}, 9},                       0, 0, true},
    {"kr_coastline",    "한반도 해안선",   8,  {QColor(0xc8, 0xd3, 0xe0), 1.4, Qt::SolidLine, {}, {}, 9},                      0, 0, true},
    {"kr_dmz",          "비무장지대",      9,  {QColor(0xb0, 0x70, 0x40), 0.8, Qt::DotLine, {}, {}, 9},                        0.8, 0, true},
    {"kr_mdl",          "군사분계선",      10, {QColor(0xff, 0x5a, 0x4f), 1.8, Qt::DashDotLine, {}, {}, 9},                    0, 0, true},
    {"kr_mountains",    "산",              11, {QColor(0xb8, 0xa2, 0x7a), 1.0, Qt::SolidLine, {}, QColor(0xb8, 0xa2, 0x7a), 8}, 0, 0.9, true},
    {"kr_islands",      "섬 이름",         12, {QColor(0xc8, 0xd3, 0xe0), 1.0, Qt::SolidLine, {}, QColor(0xe6, 0xec, 0xf3), 9,
                                               LayerStyle::PointSymbol::Circle}, 0, 0, true},
};

QString settingsKey(const QString& id)
{
    return QStringLiteral("map/layers/") + id;
}

} // namespace

MapLayerManager::MapLayerManager(QObject* parent)
    : QObject(parent)
{
}

MapLayerManager::~MapLayerManager() = default;

int MapLayerManager::loadDefaultLayers(const MapProjection& proj, Logger* logger)
{
    int loaded = 0;
    for (const LayerDef& d : kDefaults) {
        auto layer = std::make_unique<VectorLayer>(QString::fromLatin1(d.id), QString::fromUtf8(d.name));
        layer->setStyle(d.style);
        layer->setZValue(d.z);
        layer->setMinLod(d.minLod);
        layer->setLabelMinLod(d.labelMinLod);
        QString err;
        const QString path = QStringLiteral(":/map/%1.geojson").arg(QString::fromLatin1(d.id));
        if (!layer->loadFile(path, proj, &err)) {
            if (logger)
                logger->error(QStringLiteral("지도 레이어 로드 실패 [%1]: %2").arg(layer->displayName(), err),
                              QStringLiteral("Map"));
            continue;
        }
        addLayer(std::move(layer), d.visible);
        ++loaded;
    }
    addVirtualLayer(GraticuleId, QStringLiteral("위경도선"), true);
    if (logger)
        logger->info(QStringLiteral("지도 레이어 %1/%2개 로드").arg(loaded).arg(int(std::size(kDefaults))),
                     QStringLiteral("Map"));
    return loaded;
}

void MapLayerManager::addLayer(std::unique_ptr<VectorLayer> layer, bool visible)
{
    if (!layer || contains(layer->id()))
        return;
    Entry e;
    e.id = layer->id();
    e.displayName = layer->displayName();
    e.visible = visible;
    e.layer = std::move(layer);
    m_entries.push_back(std::move(e));
}

void MapLayerManager::addVirtualLayer(const QString& id, const QString& displayName, bool visible)
{
    if (contains(id))
        return;
    Entry e;
    e.id = id;
    e.displayName = displayName;
    e.visible = visible;
    m_entries.push_back(std::move(e));
}

QStringList MapLayerManager::layerIds() const
{
    QStringList ids;
    for (const Entry& e : m_entries)
        ids << e.id;
    return ids;
}

QString MapLayerManager::displayName(const QString& id) const
{
    const Entry* e = find(id);
    return e ? e->displayName : QString();
}

bool MapLayerManager::contains(const QString& id) const
{
    return find(id) != nullptr;
}

bool MapLayerManager::isVisible(const QString& id) const
{
    const Entry* e = find(id);
    return e && e->visible;
}

VectorLayer* MapLayerManager::layer(const QString& id) const
{
    const Entry* e = find(id);
    return e ? e->layer.get() : nullptr;
}

void MapLayerManager::setVisible(const QString& id, bool visible)
{
    Entry* e = find(id);
    if (!e || e->visible == visible)
        return;
    e->visible = visible;
    emit layerVisibilityChanged(id, visible);
}

void MapLayerManager::restoreVisibility()
{
    QSettings s;
    for (const Entry& e : m_entries) {
        const QVariant v = s.value(settingsKey(e.id));
        if (v.isValid())
            setVisible(e.id, v.toBool());
    }
}

void MapLayerManager::saveVisibility() const
{
    QSettings s;
    for (const Entry& e : m_entries)
        s.setValue(settingsKey(e.id), e.visible);
}

const MapLayerManager::Entry* MapLayerManager::find(const QString& id) const
{
    for (const Entry& e : m_entries)
        if (e.id == id)
            return &e;
    return nullptr;
}

MapLayerManager::Entry* MapLayerManager::find(const QString& id)
{
    return const_cast<Entry*>(std::as_const(*this).find(id));
}
