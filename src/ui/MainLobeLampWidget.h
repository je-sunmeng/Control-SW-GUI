#pragma once

#include "common/Types.h"

#include <QSet>
#include <QVector>
#include <QWidget>

class QLabel;
class LampIndicator;
class ScenarioModel;

// 레이다별 MainLobe 램프 (최대 4)
//  - 순서 = 오브젝트 목록의 레이다 순서 (삭제 시 뒤 레이다가 앞으로 당겨짐), 램프 색 = 레이다 색
//  - 단독모드: MainLobe(스캔주기마다 내부 생성) 시 깜빡임(펄스)
//  - 연동모드: 최초 MainLobe 수신 시점부터 계속 켜짐 (래치)
//  - MainLobe 를 아직 받지 않은 레이다 이름은 어둡게
class MainLobeLampWidget : public QWidget {
    Q_OBJECT
public:
    static constexpr int SlotCount = 4;

    explicit MainLobeLampWidget(ScenarioModel& model, QWidget* parent = nullptr);

    // 테스트/진단용
    ObjectId radarAt(int slot) const { return slot >= 0 && slot < m_order.size() ? m_order[slot] : 0; }
    bool isActive(ObjectId radarId) const { return m_active.contains(radarId); }
    bool isLampOn(int slot) const;
    bool isLinked() const { return m_linked; }

public slots:
    void onMainLobe(ObjectId radarId);
    void refreshRadars();
    void setLinked(bool linked);
    void resetMission();   // 새 임무 전: 모두 소등, 이름 어둡게

private:
    void updateSlot(int slot);

    ScenarioModel& m_model;
    LampIndicator* m_lamps[SlotCount];
    QLabel* m_labels[SlotCount];
    QVector<ObjectId> m_order;   // 슬롯 → 레이다 ID (목록 순서)
    QSet<ObjectId> m_active;     // 이번 임무에서 MainLobe 를 받은 레이다
    bool m_linked = false;
};
