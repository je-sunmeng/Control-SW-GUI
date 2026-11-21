# Control-SW-GUI
Make Gui by Qt

전자전기(Stand Off Jammer)-레이다 시연용 제어 SW (Qt6 / C++17 / CMake)

- 설계서: [docs/DESIGN.md](docs/DESIGN.md)
- 프로토콜 확장 가이드: [docs/PROTOCOL_GUIDE.md](docs/PROTOCOL_GUIDE.md)

## 진행 상태

| 단계 | 상태 | 내용 |
|---|---|---|
| M1 | ✅ 완료 | 프로젝트 골격, MainWindow 4분할 레이아웃(도크·레이아웃 저장), Logger(스레드 안전·일자별 파일)/LogPanel(유형 필터·검색·자동 스크롤) |
| M2 | ⏳ | 지도 (투영, GeoJSON 레이어, 줌/팬, 레이어 토글) |
| M3~M6 | ⏳ | 설계서 9장 참고 |

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

로그 파일 위치: `QStandardPaths::AppLocalDataLocation/logs/yyyyMMdd.log`
(macOS: `~/Library/Application Support/CtrlSW/CtrlSW/logs`, Windows: `%LOCALAPPDATA%\CtrlSW\CtrlSW\logs`)
