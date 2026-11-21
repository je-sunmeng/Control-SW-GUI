#pragma once

#include <QFrame>

// M1 중앙 영역 자리표시자. M2에서 MapView(QGraphicsView)로 교체.
class MapPlaceholder : public QFrame {
    Q_OBJECT
public:
    explicit MapPlaceholder(QWidget* parent = nullptr);
};
