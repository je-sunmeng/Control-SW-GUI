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
| M5 | ✅ 완료 | 네트워크·연동모드: TCP 서버, 프레임(BigEndian 헤더 8B), `protocol.json` 매핑, 메시지 10종, 장치 식별·연결 상태, 임무전송/START/STOP, 위치 주기송신, MainLobe 수신 → 빔 동기화·탐지 전시 ([ICD 초안](docs/ICD.md)) |
| M6 | ✅ 완료 | 모의 클라이언트 `ctrlsw_mock`(분석SW/신호발생장치 에뮬레이터), GitHub Actions: Windows(MSVC)·macOS × Qt 6.8 LTS 빌드·테스트, windeployqt 배포 아티팩트 |

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
%QT_PREFIX%\bin\windeployqt build\windows-msvc\Release\CtrlSW.exe
```

### Windows 배포본

**[Releases](https://github.com/je-sunmeng/Control-SW-GUI/releases)** 에서 `CtrlSW-<버전>-windows-x64.zip` 을 받아 압축을 풀면
(브랜치 빌드는 Actions → `build` 의 `CtrlSW-windows-x64` 아티팩트)
`CtrlSW.exe`, `ctrlsw_mock.exe`, Qt DLL, `samples/` 가 포함된 실행 폴더입니다 (Qt 설치 불필요).
`protocol.json.example` 을 `protocol.json` 으로 이름을 바꿔 두면 Command Code 매핑을 덮어쓸 수 있습니다.

릴리스 절차: `CMakeLists.txt` 의 `VERSION` 갱신 → `docs/release/v<버전>.md` 작성 → `git tag v<버전> && git push origin v<버전>`
→ CI 가 Windows 빌드·테스트·스모크 테스트 후 GitHub Release 에 zip 게시

### 실행 옵션

| 옵션 | 설명 |
|---|---|
| `--screenshot <file.png>` | 메인 창을 PNG로 저장 후 종료 (원격 개발/점검용) |
| `--map-zoom <px/km>` | 시작 시 지도 배율 지정 (개발용) |
| `--scenario <file>` | 시작 시 시나리오 불러오기 (예: `samples/demo.scenario.json`) |
| `--start` / `--speed <1\|2\|4\|8>` | 단독모드 임무 자동 시작 / 배속 지정 |
| `--screenshot-delay <ms>` | 스크린샷 지연 (기본 800) |
| `--linked` / `--port <n>` | 시작 시 연동모드(TCP 서버 대기) / 서버 포트 (기본 50021) |

시연 예: `CtrlSW --scenario samples/demo.scenario.json --speed 4 --start`

## 지도 조작

| 조작 | 동작 |
|---|---|
| 마우스 휠 / `+` `-` | 커서 기준 확대·축소 (최대 축소 = 지도 전체가 화면을 채우는 크기) |
| 좌클릭 드래그 (빈 곳) | 지도 이동 (휠클릭 드래그도 가능) |
| `Home` / [한반도 중심] | 한반도 전체 보기 |
| [레이어 ▾] | 레이어 표시 토글 (다음 실행 시 유지) |

## 시나리오 편집

| 작업 | 방법 |
|---|---|
| 레이다 추가 | [레이다 추가] → 지도 클릭 (최대 4, 색상 R1~R4 고정) |
| 전자전기/아군편대/이동표적 동선 | [추가] → 지도 클릭으로 웨이포인트 → 더블클릭·Enter 로 완료, **우클릭**/Backspace: 가장 최근 점 삭제, Esc: 취소 |
| 전자전기 동선 | **루프**: 마지막 점 → 시작점 구간이 자동 연결되고 임무 중 반복 비행 |
| 고정/잡음 표적 위치 | [단독허위표적 추가] → 타입 선택 → 지도 클릭 |
| 위치 수정 | 지도에서 심볼/웨이포인트 드래그 또는 설정 패널에서 수치 입력 (마우스 휠은 값 대신 패널 스크롤) |
| 삭제 | 목록에서 Delete 키 또는 우클릭 메뉴 |
| 저장/불러오기 | 항목별 `*.radar.json`, `*.jammer.json`, `*.formation.json`, `*.falsetarget.json`, 전체 `*.scenario.json` (불러오기 = 기존 항목 교체, ID 재발급) |

지도 데이터: [Natural Earth](https://www.naturalearthdata.com/) (Public Domain), 생성 방법은 [tools/map_prep](tools/map_prep/README.md)

## 연동 시험 (모의 클라이언트)

```bash
# 1) 제어SW 를 연동모드로 실행 (또는 화면에서 [연동] 선택)
CtrlSW --scenario samples/demo.scenario.json --linked
# 2) 분석SW + 신호발생장치 에뮬레이터 접속
ctrlsw_mock --host 127.0.0.1 --port 50021 --device both
# 3) 제어SW 에서 [임무전송] → [시작]
#    → 신호발생장치 에뮬레이터가 레이다별 스캔주기마다 MainLobeNotify(+탐지결과) 송신
```
`--device analysis|siggen` 으로 한쪽만, 다른 PC 에서 `--host <제어SW IP>` 로 접속 가능. 제어SW 에 `--start` 를 주면 두 장치 연결 시 임무전송·시작을 자동 수행.

## 프로토콜 (Command Code)

- 기본 매핑: `resources/protocol/protocol.json` (리소스 내장). **실행파일 옆에 `protocol.json` 을 두면 그 파일이 우선** (재빌드 불필요)
- 메시지 추가/변경 절차: [docs/PROTOCOL_GUIDE.md](docs/PROTOCOL_GUIDE.md), 필드 정의: [docs/ICD.md](docs/ICD.md)
- macOS 에서 연동모드 첫 실행 시 "들어오는 네트워크 연결 허용" 창이 뜨면 허용

로그 파일 위치: `QStandardPaths::AppLocalDataLocation/logs/yyyyMMdd.log`
(macOS: `~/Library/Application Support/CtrlSW/CtrlSW/logs`, Windows: `%LOCALAPPDATA%\CtrlSW\CtrlSW\logs`)
