#include "LampIndicator.h"

#include <QPainter>
#include <QRadialGradient>
#include <QVariantAnimation>

LampIndicator::LampIndicator(const QColor& onColor, int diameter, QWidget* parent)
    : QWidget(parent)
    , m_onColor(onColor)
    , m_diameter(diameter)
    , m_anim(new QVariantAnimation(this))
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_anim->setStartValue(1.0);
    m_anim->setEndValue(0.0);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_intensity = v.toDouble();
        update();
    });
}

void LampIndicator::setOnColor(const QColor& c)
{
    m_onColor = c;
    update();
}

void LampIndicator::setOn(bool on)
{
    if (m_on == on)
        return;
    m_on = on;
    update();
}

void LampIndicator::pulse(int durationMs)
{
    m_anim->stop();
    m_anim->setDuration(durationMs);
    m_intensity = 1.0;
    m_anim->start();
    update();
}

QSize LampIndicator::sizeHint() const
{
    return {m_diameter + 8, m_diameter + 8};
}

void LampIndicator::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QPointF c = rect().center() + QPointF(0.5, 0.5);
    const double level = m_on ? 1.0 : m_intensity;
    const qreal r = m_diameter / 2.0;

    // 발광 (밝기에 비례)
    if (level > 0.01) {
        QRadialGradient glow(c, r + 4);
        QColor g = m_onColor;
        g.setAlphaF(0.55 * level);
        glow.setColorAt(0.5, g);
        g.setAlphaF(0.0);
        glow.setColorAt(1.0, g);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(c, r + 4, r + 4);
    }
    // 본체: 꺼짐색 ↔ 켜짐색 보간
    const QColor off(0x3a, 0x41, 0x4b);
    QColor body;
    body.setRgbF(off.redF() + (m_onColor.redF() - off.redF()) * level,
                 off.greenF() + (m_onColor.greenF() - off.greenF()) * level,
                 off.blueF() + (m_onColor.blueF() - off.blueF()) * level);
    QRadialGradient g(c - QPointF(r * 0.3, r * 0.3), r * 1.4);
    g.setColorAt(0.0, body.lighter(150));
    g.setColorAt(1.0, body);
    p.setPen(QPen(QColor(0x14, 0x17, 0x1b), 1));
    p.setBrush(g);
    p.drawEllipse(c, r, r);
}
