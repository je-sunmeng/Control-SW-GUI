#include "ColorPalette.h"
#include <algorithm>
#include <iterator>

namespace ColorPalette {

QColor radar(int colorIndex)
{
    static const QColor kColors[] = {
        QColor(0xff, 0x6b, 0x6b),   // R1 red
        QColor(0xff, 0xa9, 0x4d),   // R2 orange
        QColor(0xff, 0xe0, 0x66),   // R3 yellow
        QColor(0xf7, 0x83, 0xac),   // R4 pink
    };
    const int n = int(std::size(kColors));
    return kColors[((colorIndex % n) + n) % n];
}

QColor jammer() { return QColor(0x63, 0xe6, 0xbe); }
QColor formation() { return QColor(0x4d, 0xab, 0xf7); }
QColor falseTarget() { return QColor(0xda, 0x77, 0xf2); }
QColor highlight() { return QColor(0xff, 0xff, 0xff); }

} // namespace ColorPalette
