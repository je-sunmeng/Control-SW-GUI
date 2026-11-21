#pragma once

#include <QWidget>

// 좌측 오브젝트 패널 컨테이너
//  ActionBar(추가/저장/불러오기) / 데이터링크 전송간격 / 오브젝트 목록 / 설정 에디터
// M1: 섹션 골격만 배치. 각 섹션 위젯은 M3에서 구현 후 교체.
class ObjectPanel : public QWidget {
    Q_OBJECT
public:
    explicit ObjectPanel(QWidget* parent = nullptr);
};
