#pragma once

#include <QColor>
#include <QWidget>

class QVariantAnimation;

// 공용 램프: 꺼짐(어두운 회색) / 켜짐(색) / 펄스(점등 후 감쇠)
class LampIndicator : public QWidget {
    Q_OBJECT
public:
    explicit LampIndicator(const QColor& onColor = QColor(0x51, 0xcf, 0x66), int diameter = 14, QWidget* parent = nullptr);

    void setOnColor(const QColor& c);
    void setOn(bool on);          // 래치 점등
    bool isOn() const { return m_on; }
    void pulse(int durationMs = 900);   // 즉시 최대 밝기 → durationMs 동안 감쇠 (TBD-7: 펄스 방식)
    double intensity() const { return m_intensity; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_onColor;
    int m_diameter;
    bool m_on = false;
    double m_intensity = 0.0;   // 펄스 밝기 0~1
    QVariantAnimation* m_anim;
};
