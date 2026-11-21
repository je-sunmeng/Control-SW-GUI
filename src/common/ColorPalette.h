#pragma once

#include <QColor>

// 지도/목록/램프에서 공통으로 쓰는 고정 색상
namespace ColorPalette {

// 레이다별 고정 색상 (colorIndex 0~3)
QColor radar(int colorIndex);

QColor jammer();
QColor formation();
QColor falseTarget();
QColor highlight();

} // namespace ColorPalette
