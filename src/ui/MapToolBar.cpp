#include "MapToolBar.h"

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

    // M2에서 MapLayerManager의 레이어 목록으로 대체
    auto* placeholder = m_layerMenu->addAction(QStringLiteral("(지도 레이어는 M2에서 추가)"));
    placeholder->setEnabled(false);

    addSeparator();
    QAction* center = addAction(QStringLiteral("한반도 중심"));
    center->setToolTip(QStringLiteral("지도를 한반도 중심으로 이동"));
    connect(center, &QAction::triggered, this, &MapToolBar::centerOnKoreaRequested);
}
