#pragma once

#include <QObject>
#include <QStringList>

#include <memory>
#include <vector>

class Logger;
class MapProjection;
class VectorLayer;

// 지도 레이어 목록 / 표시 여부 / 스타일 관리.
// 벡터 레이어 외에 데이터가 없는 레이어(위경도선 등)도 같은 방식으로 표시 토글을 관리한다.
class MapLayerManager : public QObject {
    Q_OBJECT
public:
    static inline const QString GraticuleId = QStringLiteral("graticule");

    explicit MapLayerManager(QObject* parent = nullptr);
    ~MapLayerManager() override;

    // resources/map 의 기본 레이어 로드 (실패 레이어는 Logger에 오류 기록 후 건너뜀). 로드 성공 수 반환
    int loadDefaultLayers(const MapProjection& proj, Logger* logger = nullptr);

    void addLayer(std::unique_ptr<VectorLayer> layer, bool visible = true);
    void addVirtualLayer(const QString& id, const QString& displayName, bool visible = true);

    QStringList layerIds() const;   // 등록 순서
    QString displayName(const QString& id) const;
    bool contains(const QString& id) const;
    bool isVisible(const QString& id) const;
    VectorLayer* layer(const QString& id) const;   // 가상 레이어는 nullptr

    // QSettings("map/layers/<id>")에 저장된 표시 여부 반영 / 저장
    void restoreVisibility();
    void saveVisibility() const;

public slots:
    void setVisible(const QString& id, bool visible);

signals:
    void layerVisibilityChanged(const QString& id, bool visible);

private:
    struct Entry {
        QString id;
        QString displayName;
        bool visible = true;
        std::unique_ptr<VectorLayer> layer;
    };
    const Entry* find(const QString& id) const;
    Entry* find(const QString& id);

    std::vector<Entry> m_entries;
};
