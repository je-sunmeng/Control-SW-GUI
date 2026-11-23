# Control-SW-GUI
Make Gui by Qt

전자전기(Stand Off Jammer)-레이다 시연용 제어 SW (Qt6 / C++17 / CMake)

- 설계서: [docs/DESIGN.md](docs/DESIGN.md)
- 프로토콜 확장 가이드: [docs/PROTOCOL_GUIDE.md](docs/PROTOCOL_GUIDE.md)

## 진행 상태

| 단계 | 상태 | 내용 |
|---|---|---|
| M1 | ✅ 완료 | 프로젝트 골격, MainWindow 4분할 레이아웃(도크·레이아웃 저장), Logger(스레드 안전·일자별 파일)/LogPanel(유형 필터·검색·자동 스크롤) |
| M2 | ✅ 완료 | 지도: 정거원통 투영, GeoJSON 벡터 레이어 11종 + 위경도선, 휠 줌/좌·중클릭 드래그 팬, 레이어 토글(설정 저장), 한반도 중심, 축척 막대, 커서 위경도 |
| M3 | ✅ 완료 | 시나리오 모델(레이다/전자전기/아군편대/단독허위표적), 오브젝트 목록·설정 에디터, 지도 편집(배치·동선 그리기·핸들 드래그), 항목별/시나리오 저장·불러오기 |
| M4 | ✅ 완료 | 단독모드 시뮬레이션: 이동체(대권 보간), 빔 회전(MainLobe 동기화·sinc² 로브·잔상), 편대 허위타겟, 자체 탐지 판정, 랜덤 고스트, 잡음섹터, 배속, 시작/일시정지/재개/중지/새로고침 |
| M5 | ✅ 완료 | 네트워크·연동모드: TCP 서버, 프레임(BigEndian 헤더 8B), `protocol.json` 매핑, 메시지 10종, 장치 식별·연결 상태, 임무전송/START/STOP, 위치 주기송신, MainLobe 수신 → 빔 동기화·탐지 전시 ([ICD 초안](docs/ICD.md)) |
| M6 | ✅ 완료 | 모의 클라이언트 `ctrlsw_mock`(분석SW/재밍기법발생기 에뮬레이터), GitHub Actions: Windows(MSVC, Qt 6.8 LTS)·macOS(Qt 6.10) 빌드·테스트, windeployqt 배포 아티팩트 |

## 시작하기

> `main` = 검증된 안정 버전 (릴리스 기준), 신규 개발은 별도 브랜치에서 진행 후 `main` 에 병합합니다.
> 개발 중인 코드를 받으려면 `git clone -b <브랜치명> ...` 처럼 브랜치를 지정하세요.

### 방법 A. 빌드 없이 실행 (Windows 배포본)

1. **[Releases](https://github.com/je-sunmeng/Control-SW-GUI/releases)** 에서 최신 `CtrlSW-<버전>-windows-x64.zip` 다운로드
2. 원하는 폴더에 압축 해제 (Qt·VC++ 런타임 포함, 설치 불필요)
3. `CtrlSW.exe` 더블클릭
4. 바로 시연: 명령 프롬프트에서 압축 푼 폴더로 이동 후
   ```bat
   CtrlSW.exe --scenario samples\demo.scenario.json --speed 4 --start
   ```

| 폴더 내용 | 설명 |
|---|---|
| `CtrlSW.exe` | 제어 SW |
| `ctrlsw_mock.exe` | 분석SW·재밍기법발생기 모의 클라이언트 (연동 시험용, 콘솔) |
| `samples/` | 데모 시나리오 |
| `protocol.json.example` | Command Code 덮어쓰기 예시 (`protocol.json` 으로 이름 변경 시 적용) |
| `README.txt` | 배포본 사용 안내 |

브랜치의 최신 미릴리스 빌드는 GitHub → Actions → `build` 실행 결과의 `CtrlSW-windows-x64` 아티팩트로 받을 수 있습니다 (로그인 필요).

### 방법 B. 소스를 받아 직접 빌드 — macOS

**1) 준비 (최초 1회)**
```bash
xcode-select --install                 # Xcode 명령줄 도구 (clang, git)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"   # Homebrew 미설치 시
brew install qt cmake ninja            # Qt 6, CMake ≥ 3.21, Ninja
```

**2) 소스 받기**
```bash
git clone https://github.com/je-sunmeng/Control-SW-GUI.git
cd Control-SW-GUI
```

**3) 빌드**
```bash
cmake --preset macos-debug             # 구성 (Homebrew Qt 자동 탐색)
cmake --build --preset macos-debug     # 빌드
ctest --preset macos-debug             # 단위 테스트 (선택, 20개)
```
- 다른 경로의 Qt(공식 설치 프로그램 등)를 쓰려면: `QT_PREFIX=~/Qt/6.8.3/macos cmake --preset macos-debug`
- 배포용 최적화 빌드: `macos-debug` 대신 `macos-release` (결과는 `build/macos-release/`)

**4) 실행**
```bash
open build/macos-debug/src/CtrlSW.app                          # 일반 실행
# 옵션을 주어 실행 (앱 번들 안의 실행파일을 직접 호출)
build/macos-debug/src/CtrlSW.app/Contents/MacOS/CtrlSW --scenario samples/demo.scenario.json --speed 4 --start
# 모의 클라이언트
build/macos-debug/tools/mock_client/ctrlsw_mock --device both
```
연동모드 첫 실행 시 "들어오는 네트워크 연결 허용" 창이 뜨면 **허용**.

### 방법 C. 소스를 받아 직접 빌드 — Windows

**1) 준비 (최초 1회)**

| 도구 | 설치 |
|---|---|
| Git | [git-scm.com](https://git-scm.com/download/win) |
| Visual Studio 2022 | Community 이상, 워크로드 **"C++를 사용한 데스크톱 개발"** 선택 (MSVC, CMake 포함) |
| Qt 6.8 LTS | [Qt Online Installer](https://www.qt.io/download-qt-installer) → Qt 6.8.x → **MSVC 2022 64-bit** 체크 (예: `C:\Qt\6.8.3\msvc2022_64`) |

**2) 소스 받기** — "x64 Native Tools Command Prompt for VS 2022" 를 열고
```bat
git clone https://github.com/je-sunmeng/Control-SW-GUI.git
cd Control-SW-GUI
```

**3) 빌드**
```bat
set QT_PREFIX=C:\Qt\6.8.3\msvc2022_64
cmake --preset windows-msvc
cmake --build --preset windows-release
```
테스트까지 돌리려면 `cmake --build --preset windows-debug` 후 `ctest --preset windows-debug`.

**4) 실행 준비 (Qt DLL 복사)** — 빌드 결과 exe 는 Qt DLL 이 없으면 실행되지 않으므로 한 번 배포 도구를 돌립니다
```bat
%QT_PREFIX%\bin\windeployqt --release build\windows-msvc\Release\CtrlSW.exe
```

**5) 실행**
```bat
build\windows-msvc\Release\CtrlSW.exe
build\windows-msvc\Release\CtrlSW.exe --scenario samples\demo.scenario.json --speed 4 --start
build\windows-msvc\Release\ctrlsw_mock.exe --device both
```
Windows 방화벽 허용 창이 뜨면 **허용** (연동모드 TCP 서버).

> Qt Creator 를 쓰는 경우: [파일 → 파일 또는 프로젝트 열기] 로 루트의 `CMakeLists.txt` 를 열고 Kit(Qt 6.8 MSVC 2022 64bit / macOS) 선택 → 빌드·실행(▶).

### 최신 코드로 업데이트

```bash
cd Control-SW-GUI
git pull                               # 최신 변경 받기
cmake --build --preset macos-debug     # (Windows: cmake --build --preset windows-release)
```
CMakeLists 변경 등으로 빌드가 꼬이면 `build/` 폴더를 지우고 3) 부터 다시 수행합니다.

### 문제 해결

| 증상 | 조치 |
|---|---|
| `Could not find a package configuration file provided by "Qt6"` | Qt 경로 지정: macOS `QT_PREFIX=$(brew --prefix qt)`, Windows `set QT_PREFIX=C:\Qt\6.8.3\msvc2022_64` 후 구성 다시 실행 (`build/` 삭제 권장) |
| Windows 에서 `Qt6Core.dll 을 찾을 수 없습니다` | 4) `windeployqt` 실행, 또는 `%QT_PREFIX%\bin` 을 PATH 에 추가 |
| `No such preset` | 해당 OS 용 프리셋 사용 (macOS `macos-*`, Windows `windows-*`), CMake 3.21 이상 확인 (`cmake --version`) |
| 지도·화면이 비어 있음 | 실행 로그 확인: 앱 하단 로그창 또는 아래 "로그 파일 위치" |
| 모의 클라이언트가 접속 안 됨 | CtrlSW 가 연동모드인지(화면 [연동] 또는 `--linked`), 포트(기본 50021)·방화벽 확인 |

### 릴리스 절차 (관리자)

`CMakeLists.txt` 의 `VERSION` 갱신 → `docs/release/v<버전>.md` 작성 → `git tag -a v<버전> -m ... && git push origin v<버전>`
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
| 저장 위치 | 기본 폴더 `문서/CtrlSW` 아래 **종류별 하위 폴더**(`레이다/`, `전자전기/`, `아군편대/`, `단독허위표적/`, `시나리오/`), 기본 파일명 `<종류>_<날짜>_<시각>`. 기본 폴더는 파일 → 기본 저장 폴더 설정… 으로 변경 |
| 데이터링크 전송간격 | 연동모드에서 제어 패널(장치 연결 상태 아래)에 표시: 전자전기 / 아군편대 위치 전송간격 |
| 저장/불러오기 | 항목별 `*.radar.json`, `*.jammer.json`, `*.formation.json`, `*.falsetarget.json`, 전체 `*.scenario.json` (불러오기 = 기존 항목 교체, ID 재발급) |

지도 데이터: [Natural Earth](https://www.naturalearthdata.com/) (Public Domain), 생성 방법은 [tools/map_prep](tools/map_prep/README.md)

## 연동 시험 (모의 클라이언트)

```bash
# 1) 제어SW 를 연동모드로 실행 (또는 화면에서 [연동] 선택)
CtrlSW --scenario samples/demo.scenario.json --linked
# 2) 분석SW + 재밍기법발생기 에뮬레이터 접속
ctrlsw_mock --host 127.0.0.1 --port 50021 --device both
# 3) 제어SW 에서 [임무전송] → [시작]
#    → 재밍기법발생기 에뮬레이터가 레이다별 스캔주기마다 MainLobeNotify(+탐지결과) 송신
```
`--device analysis|siggen` 으로 한쪽만, 다른 PC 에서 `--host <제어SW IP>` 로 접속 가능. 제어SW 에 `--start` 를 주면 두 장치 연결 시 임무전송·시작을 자동 수행.

## 프로토콜 (Command Code)

- 기본 매핑: `resources/protocol/protocol.json` (리소스 내장). **실행파일 옆에 `protocol.json` 을 두면 그 파일이 우선** (재빌드 불필요)
- 메시지 추가/변경 절차: [docs/PROTOCOL_GUIDE.md](docs/PROTOCOL_GUIDE.md), 필드 정의: [docs/ICD.md](docs/ICD.md)
- macOS 에서 연동모드 첫 실행 시 "들어오는 네트워크 연결 허용" 창이 뜨면 허용

로그 파일 위치: `QStandardPaths::AppLocalDataLocation/logs/yyyyMMdd.log`
(macOS: `~/Library/Application Support/CtrlSW/CtrlSW/logs`, Windows: `%LOCALAPPDATA%\CtrlSW\CtrlSW\logs`)
