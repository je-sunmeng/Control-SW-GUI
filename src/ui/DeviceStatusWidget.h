#pragma once

#include "common/Types.h"

#include <QWidget>

class QLabel;
class LampIndicator;

// 분석SW / 재밍기법발생기 연결 상태 (M5 에서 NetworkManager 와 연결)
class DeviceStatusWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceStatusWidget(QWidget* parent = nullptr);

public slots:
    void setConnected(DeviceCode dev, bool connected);
    void setNetworkActive(bool active);   // 단독모드 = 비활성(회색 안내)
    void setListening(bool listening, quint16 port);

private:
    struct Row {
        LampIndicator* lamp;
        QLabel* state;
    };
    Row m_analysis;
    Row m_signalGen;
    QLabel* m_server;
    bool m_active = false;
};
