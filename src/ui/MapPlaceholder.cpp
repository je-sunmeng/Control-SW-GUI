#include "MapPlaceholder.h"

#include <QLabel>
#include <QVBoxLayout>

MapPlaceholder::MapPlaceholder(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("MapPlaceholder"));
    setFrameShape(QFrame::StyledPanel);
    setMinimumSize(400, 300);

    auto* label = new QLabel(QStringLiteral("지도 영역\n(MapView — M2)"), this);
    label->setObjectName(QStringLiteral("placeholderLabel"));
    label->setAlignment(Qt::AlignCenter);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(label);
}
