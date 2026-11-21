# Control-SW-GUI
Make Gui by Qt

전자전기(Stand Off Jammer)-레이다 시연용 제어 SW (Qt6 / C++17 / CMake)

- 설계서: [docs/DESIGN.md](docs/DESIGN.md)
- 프로토콜 확장 가이드: [docs/PROTOCOL_GUIDE.md](docs/PROTOCOL_GUIDE.md)

## 진행 상태

| 단계 | 상태 | 내용 |
|---|---|---|
| M1 | ✅ 완료 | 프로젝트 골격, MainWindow 4분할 레이아웃(도크·레이아웃 저장), Logger(스레드 안전·일자별 파일)/LogPanel(유형 필터·검색·자동 스크롤) |
| M2 | ✅ 완료 | 지도: 정거원통 투영, GeoJSON 벡터 레이어 10종 + 위경도선, 휠 줌/우·중클릭 드래그 팬, 레이어 토글(설정 저장), 한반도 중심, 축척 막대, 커서 위경도 |
| M3 | ✅ 완료 | 시나리오 모델(레이다/전자전기/아군편대/단독허위표적), 오브젝트 목록·설정 에디터, 지도 편집(배치·동선 그리기·핸들 드래그), 항목별/시나리오 저장·불러오기 |
| M4 | ✅ 완료 | 단독모드 시뮬레이션: 이동체(대권 보간), 빔 회전(MainLobe 동기화·sinc² 로브·잔상), 편대 허위타겟, 자체 탐지 판정, 랜덤 고스트, 잡음섹터, 배속, 시작/일시정지/재개/중지/새로고침 |
| M5~M6 | ⏳ | 설계서 9장 참고 |

## 빌드

### macOS (Homebrew)

```bash
brew install qt cmake ninja
cmake --preset macos-debug          # Homebrew Qt는 자동 탐색. 다른 Qt 사용 시 QT_PREFIX=/path/to/Qt/6.x/macos
cmake --build --preset macos-debug
ctest --preset macos-debug          # 단위 테스트
open build/macos-debug/src/CtrlSW.app
```

### Windows (MSVC 2022)

```bat
set QT_PREFIX=C:\Qt\6.8.3\msvc2022_64
cmake --preset windows-msvc
cmake --build --preset windows-release
ctest --preset windows-debug
%QT_PREFIX%\bin\windeployqt build\windows-msvc\src\Release\CtrlSW.exe
```

### 실행 옵션

| 옵션 | 설명 |
|---|---|
| `--screenshot <file.png>` | 메인 창을 PNG로 저장 후 종료 (원격 개발/점검용) |
| `--map-zoom <px/km>` | 시작 시 지도 배율 지정 (개발용) |
| `--scenario <file>` | 시작 시 시나리오 불러오기 (예: `samples/demo.scenario.json`) |
| `--start` / `--speed <1\|2\|4\|8>` | 단독모드 임무 자동 시작 / 배속 지정 |
| `--screenshot-delay <ms>` | 스크린샷 지연 (기본 800) |

시연 예: `CtrlSW --scenario samples/demo.scenario.json --speed 4 --start`

## 지도 조작

| 조작 | 동작 |
|---|---|
| 마우스 휠 / `+` `-` | 커서 기준 확대·축소 |
| 우클릭 또는 휠클릭 드래그 | 지도 이동 |
| `Home` / [한반도 중심] | 한반도 전체 보기 |
| [레이어 ▾] | 레이어 표시 토글 (다음 실행 시 유지) |

## 시나리오 편집

| 작업 | 방법 |
|---|---|
| 레이다 추가 | [탐지기 추가] → 지도 클릭 (최대 4, 색상 R1~R4 고정) |
| 전자전기/아군편대/이동표적 동선 | [추가] → 지도 클릭으로 웨이포인트 → 더블클릭·Enter·우클릭으로 완료, Backspace: 마지막 점 삭제, Esc: 취소 |
| 고정/잡음 표적 위치 | [단독허위표적 추가] → 타입 선택 → 지도 클릭 |
| 위치 수정 | 지도에서 심볼/웨이포인트 드래그 또는 설정 패널에서 수치 입력 |
| 삭제 | 목록에서 Delete 키 또는 우클릭 메뉴 |
| 저장/불러오기 | 항목별 `*.radar.json`, `*.jammer.json`, `*.formation.json`, `*.falsetarget.json`, 전체 `*.scenario.json` (불러오기 = 기존 항목 교체, ID 재발급) |

지도 데이터: [Natural Earth](https://www.naturalearthdata.com/) (Public Domain), 생성 방법은 [tools/map_prep](tools/map_prep/README.md)

로그 파일 위치: `QStandardPaths::AppLocalDataLocation/logs/yyyyMMdd.log`
(macOS: `~/Library/Application Support/CtrlSW/CtrlSW/logs`, Windows: `%LOCALAPPDATA%\CtrlSW\CtrlSW\logs`)
