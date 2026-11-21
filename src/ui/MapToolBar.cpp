#include "MapToolBar.h"

#include "map/MapLayerManager.h"

#include <QAction>
#include <QMenu>
#include <QToolButton>

MapToolBar::MapToolBar(QWidget* parent)
    : QToolBar(QStringLiteral("지도"), parent)
    , m_layerMenu(new QMenu(QStringLiteral("레이어"), this))
{
    setObjectName(QStringLiteral("MapToolBar"));
    setMovable(false);
    setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto* layerBtn = new QToolButton(this);
    layerBtn->setText(QStringLiteral("레이어 ▾"));
    layerBtn->setToolTip(QStringLiteral("지도 레이어 표시 설정"));
    layerBtn->setMenu(m_layerMenu);
    layerBtn->setPopupMode(QToolButton::InstantPopup);
    addWidget(layerBtn);

    addSeparator();
    QAction* center = addAction(QStringLiteral("한반도 중심"));
    center->setToolTip(QStringLiteral("지도를 한반도 중심으로 이동 (Home)"));
    connect(center, &QAction::triggered, this, &MapToolBar::centerOnKoreaRequested);

    rebuildLayerMenu();
}

void MapToolBar::setLayerManager(MapLayerManager* layers)
{
    if (m_layers)
        disconnect(m_layers, nullptr, this, nullptr);
    m_layers = layers;
    if (m_layers) {
        connect(m_layers, &MapLayerManager::layerVisibilityChanged, this, [this](const QString& id, bool on) {
            if (QAction* a = m_layerActions.value(id)) {
                const QSignalBlocker block(a);
                a->setChecked(on);
            }
        });
    }
    rebuildLayerMenu();
}

void MapToolBar::rebuildLayerMenu()
{
    m_layerMenu->clear();
    m_layerActions.clear();
    if (!m_layers || m_layers->layerIds().isEmpty()) {
        m_layerMenu->addAction(QStringLiteral("(지도 레이어 없음)"))->setEnabled(false);
        return;
    }

    // 위에 그려지는 레이어가 메뉴 위쪽에 오도록 역순 (위경도선은 마지막)
    QStringList ids = m_layers->layerIds();
    std::reverse(ids.begin(), ids.end());
    ids.removeAll(MapLayerManager::GraticuleId);
    if (m_layers->contains(MapLayerManager::GraticuleId))
        ids << MapLayerManager::GraticuleId;

    for (const QString& id : std::as_const(ids)) {
        if (id == MapLayerManager::GraticuleId)
            m_layerMenu->addSeparator();
        QAction* a = m_layerMenu->addAction(m_layers->displayName(id));
        a->setCheckable(true);
        a->setChecked(m_layers->isVisible(id));
        connect(a, &QAction::toggled, m_layers, [layers = m_layers, id](bool on) {
            layers->setVisible(id, on);
            layers->saveVisibility();
        });
        m_layerActions.insert(id, a);
    }

    m_layerMenu->addSeparator();
    m_layerMenu->addAction(QStringLiteral("모두 표시"), this, [this] {
        for (const QString& id : m_layers->layerIds())
            m_layers->setVisible(id, true);
        m_layers->saveVisibility();
    });
}
