# 전자전기-레이다 시연 GUI (제어 SW) — 파일 및 클래스 설계서

> 상태: **설계 단계 (v0.2)** — 코드/프로젝트 생성은 Macbook에서 프로젝트 경로 지정 후 착수
> 대상: Qt 6 (C++17), CMake, Widgets + Network

---

## 1. 개요

| 항목 | 내용 |
|---|---|
| 목표 | Stand Off Jammer 전자전기를 이용한 아군 편대 보호 시나리오용 레이다 시뮬레이터 제어 SW |
| 개발 | Macbook Air 15" (주 개발, claude remote-control) |
| 점검 | HP OMEN Laptop (GitHub clone 후 빌드·기능 점검, Windows) |
| 실행 | Lenovo ThinkPad P16 Gen3 (Windows 가정) |
| 프레임워크 | Qt 6.5+ LTS (Core, Gui, Widgets, Network), CMake 3.21+ |
| 언어 | C++17 |
| 크로스플랫폼 | macOS(clang) / Windows(MSVC 2022) 모두 빌드 가능하도록 플랫폼 의존 코드 금지 |

### 1.1 설계 원칙
1. **계층 분리**: Model(데이터) / Map(전시) / Sim(시뮬레이션 엔진) / Net(통신) / UI(위젯) / Persistence(저장).
2. **UI는 Model을 직접 수정하지 않고** `ScenarioModel`의 API와 시그널을 통해서만 변경 → 지도·목록·설정패널 자동 동기화.
3. **메시지 확장성**: Command Code 값은 외부 매핑 파일(`protocol.json`)로 분리하고, 메시지는 `IMessage` 인터페이스 + `MessageFactory` 레지스트리로 추가.
4. **모드 독립성**: 단독모드에서는 `NetworkManager`가 아예 동작하지 않으며, MainLobe 시점/탐지 결과는 엔진이 내부 생성. 연동모드에서는 신호발생장치의 MainLobe 탐지결과 메시지를 그대로 사용.
5. **시간 관리 일원화**: 모든 이동/빔 회전은 `SimClock`의 시뮬레이션 시간(sec)으로 계산(배속은 Clock만 영향).
6. **일시정지는 단독모드 전용**: 연동모드에서는 일시정지/재개 불가(타 장비로 일시정지 명령 송신 없음).

---

## 2. 디렉토리 / 파일 구조

```
Control-SW-GUI/
├── CMakeLists.txt                     # 루트 (Qt6 find_package, 하위 디렉토리 추가)
├── CMakePresets.json                  # macos-debug / windows-msvc-debug/release 프리셋
├── README.md
├── docs/
│   ├── DESIGN.md                      # 본 문서
│   ├── PROTOCOL_GUIDE.md              # Command Code/메시지 확장 설계 및 작성 가이드
│   └── ICD.md                         # 메시지 인터페이스 정의서 (타 장비 협의 후 작성)
├── resources/
│   ├── resources.qrc
│   ├── icons/                         # 버튼/오브젝트 아이콘 (svg)
│   ├── styles/dark.qss                # 다크 테마 스타일시트
│   ├── protocol/protocol.json         # Command Code 매핑 기본값 (실행파일 옆 파일로 덮어쓰기 가능)
│   └── map/                           # 벡터지도 데이터 (GeoJSON, 아래 7장 참고)
│       ├── coastline_world.geojson
│       ├── countries.geojson
│       ├── kr_admin1.geojson          # 시/도 경계
│       ├── kr_admin2.geojson          # 시/군/구 경계 (선택)
│       ├── kr_rivers.geojson
│       ├── kr_mountains.geojson       # 산 정상 포인트 + 주요 산맥 라인
│       └── kr_mdl.geojson             # 군사분계선
├── tools/
│   └── map_prep/                      # 지도 원본 → 경량 GeoJSON 변환 스크립트 (Python, ogr2ogr/mapshaper)
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── MainWindow.h/.cpp          # 전체 레이아웃 조립, 컴포넌트 연결
│   │   └── AppContext.h/.cpp          # 싱글톤성 객체 소유(Model, Engine, Network, Logger)
│   ├── common/
│   │   ├── Types.h                    # GeoPoint, enum(ObjectType, FalseTargetType, ScanDir, SimState, RunMode…)
│   │   ├── GeoUtils.h/.cpp            # 거리/방위/목적지점(대권, WGS84 구면 근사)
│   │   ├── IdGenerator.h/.cpp         # 오브젝트 ID 자동 생성
│   │   ├── ColorPalette.h/.cpp        # 레이다별 고정 색상(최대 4)
│   │   └── Constants.h                # 장치코드, 제한값(레이다 4, 허위타겟 8 등)
│   ├── model/
│   │   ├── SimObject.h/.cpp           # 추상 베이스 (id, name, type)
│   │   ├── Route.h/.cpp               # Waypoint 목록 + Segment(고도, 속력)
│   │   ├── Radar.h/.cpp
│   │   ├── JammerAircraft.h/.cpp      # 전자전기
│   │   ├── FriendlyFormation.h/.cpp   # 아군편대 (+ JammingConfig)
│   │   ├── JammingConfig.h/.cpp       # 교란설정 (허위타겟 N개 + 잡음신호폭)
│   │   ├── StandaloneFalseTarget.h/.cpp # 단독 허위 표적 (타입별 데이터)
│   │   └── ScenarioModel.h/.cpp       # 전체 오브젝트 컨테이너 + 변경 시그널
│   ├── persistence/
│   │   ├── JsonSerializer.h/.cpp      # 각 모델 ↔ QJsonObject
│   │   └── ObjectFileIO.h/.cpp        # 오브젝트 항목별 저장/불러오기 (*.radar.json 등)
│   ├── map/
│   │   ├── MapProjection.h/.cpp       # 위경도 ↔ Scene 좌표 (Equirectangular, cos(φ0) 보정)
│   │   ├── VectorLayer.h/.cpp         # GeoJSON 로드 → QPainterPath 캐시
│   │   ├── MapLayerManager.h/.cpp     # 레이어 목록/표시 여부/스타일
│   │   ├── GraticuleLayer.h/.cpp      # 위경도선 (줌 레벨별 간격)
│   │   ├── MapView.h/.cpp             # QGraphicsView: 줌/팬/클릭→위경도, 편집모드
│   │   ├── MapScene.h/.cpp            # QGraphicsScene: 레이어 + 오브젝트 아이템 관리
│   │   ├── MapEditController.h/.cpp   # 편집 상태머신(레이다 배치, 동선 클릭, 드래그)
│   │   └── items/
│   │       ├── RadarItem.h/.cpp           # 레이다 심볼 + 최대탐지거리 원
│   │       ├── RadarBeamItem.h/.cpp       # 빔(메인로브+사이드로브 형상) 회전 전시
│   │       ├── RouteItem.h/.cpp           # 동선 폴리라인 + 웨이포인트 핸들
│   │       ├── WaypointHandleItem.h/.cpp  # 드래그 가능한 클릭점
│   │       ├── PlatformItem.h/.cpp        # 전자전기/아군편대/이동표적 현재위치 심볼
│   │       ├── FalseTargetItem.h/.cpp     # 허위표적 심볼 (편대 허위타겟, 고정/이동)
│   │       ├── DetectionMarkItem.h/.cpp   # 레이다 탐지 결과 마커(레이다 색상)
│   │       ├── RandomGhostItem.h/.cpp     # 랜덤 허위표적: 옅은 반원 fade in/out
│   │       └── NoiseSectorItem.h/.cpp     # 잡음 허위표적: 레이다별 탐지불가 섹터
│   ├── sim/
│   │   ├── SimClock.h/.cpp            # 시뮬레이션 시간, 배속, 일시정지
│   │   ├── SimulationEngine.h/.cpp    # 틱 루프, 상태머신, 각 연산기 호출
│   │   ├── RouteKinematics.h/.cpp     # Route + t → 위치/고도/방위
│   │   ├── RadarScanner.h/.cpp        # MainLobe 기준 빔 방위 계산
│   │   ├── FalseTargetGenerator.h/.cpp# 편대 허위타겟/랜덤 고스트 위치 생성
│   │   ├── IDetectionSource.h         # 탐지 결과 공급 인터페이스
│   │   ├── LocalDetectionSource.h/.cpp# 단독모드: 자체 탐지 판정(거리, 빔, 잡음섹터)
│   │   ├── ExternalDetectionSource.h/.cpp # 연동모드: 수신 MainLobe 탐지결과 메시지 반영
│   │   └── SimSnapshot.h              # 한 틱의 결과(전시용 값 묶음)
│   ├── net/
│   │   ├── MessageHeader.h/.cpp       # Opcode(32)+DataSize(32), BigEndian
│   │   ├── Opcode.h/.cpp              # Src/Dst/Cmd 패킹/해석 (숫자 하드코딩 없음)
│   │   ├── MessageId.h                # 내부 논리 메시지 ID enum (Command Code와 분리)
│   │   ├── ProtocolTable.h/.cpp       # protocol.json 로드: MessageId ↔ (Src, Dst, Cmd) 매핑
│   │   ├── ByteWriter.h/.cpp          # QDataStream(BigEndian) 래퍼: u8/u16/u32/f32/f64/string
│   │   ├── ByteReader.h/.cpp
│   │   ├── IMessage.h                 # 메시지 인터페이스
│   │   ├── MessageFactory.h/.cpp      # MessageId → 생성자 레지스트리
│   │   ├── MessageRegistry.cpp        # 모든 메시지 클래스 등록 (새 메시지 = 1줄 추가)
│   │   ├── messages/                  # 메시지별 클래스 (2.1 참고)
│   │   ├── FrameDecoder.h/.cpp        # TCP 스트림 → 프레임 분리(부분수신 처리)
│   │   ├── ClientSession.h/.cpp       # 접속 클라이언트 1개(QTcpSocket), 장치 식별
│   │   ├── TcpServer.h/.cpp           # QTcpServer, 세션 관리
│   │   └── NetworkManager.h/.cpp      # 장치별 송신 API, 수신 디스패치, 연결상태 시그널
│   ├── mission/
│   │   └── MissionController.h/.cpp   # 버튼 동작 ↔ 엔진/네트워크 오케스트레이션
│   ├── log/
│   │   ├── LogEntry.h
│   │   ├── Logger.h/.cpp              # 로그 수집(스레드 안전) + 파일 기록
│   │   └── LogTableModel.h/.cpp       # QAbstractTableModel
│   └── ui/
│       ├── MapToolBar.h/.cpp          # 상단: 레이어 설정, "한반도 중심" 버튼
│       ├── ObjectPanel.h/.cpp         # 좌측 전체 컨테이너
│       ├── ObjectActionBar.h/.cpp     # 오브젝트별 추가/저장/불러오기 버튼
│       ├── ObjectTreeModel.h/.cpp     # 오브젝트 목록 모델 (그룹: 레이다/전자전기/편대/단독표적)
│       ├── ObjectListView.h/.cpp      # 목록 + 삭제(우클릭/Del키)
│       ├── editors/
│       │   ├── ObjectEditorStack.h/.cpp   # 선택 항목 타입에 따른 QStackedWidget
│       │   ├── RadarEditor.h/.cpp
│       │   ├── RouteSegmentTable.h/.cpp   # 구간별 고도/속력 테이블 (공용)
│       │   ├── JammerEditor.h/.cpp
│       │   ├── FormationEditor.h/.cpp     # 편대규모, 교란설정 포함
│       │   ├── JammingConfigEditor.h/.cpp # 허위타겟 테이블(이격각도, 초기이격거리), 잡음신호폭
│       │   └── FalseTargetEditor.h/.cpp   # 타입별 입력 폼
│       ├── dialogs/
│       │   └── FalseTargetTypeDialog.h/.cpp # 단독허위표적 추가 시 타입 선택
│       ├── ControlPanel.h/.cpp        # 우측 전체 컨테이너
│       ├── ModeSelector.h/.cpp        # 단독/연동 라디오
│       ├── DeviceStatusWidget.h/.cpp  # 분석SW/신호발생장치 연결 상태
│       ├── MainLobeLampWidget.h/.cpp  # 레이다별 램프(수신 시 점등 후 감쇠)
│       ├── SimControlWidget.h/.cpp    # 상태 표기 + 임무전송/시작(재개)/일시정지(단독 전용)/중지/새로고침
│       ├── SpeedSelector.h/.cpp       # 1/2/4/8 배속
│       ├── LampIndicator.h/.cpp       # 공용 램프 위젯
│       └── LogPanel.h/.cpp            # 하단 로그 테이블 + 필터
└── tests/
    ├── CMakeLists.txt                 # Qt Test
    ├── tst_Opcode.cpp
    ├── tst_MessageCodec.cpp           # 직렬화 왕복/BigEndian 검증
    ├── tst_FrameDecoder.cpp           # 부분/병합 수신
    ├── tst_GeoUtils.cpp
    ├── tst_RouteKinematics.cpp
    ├── tst_RadarScanner.cpp
    ├── tst_LocalDetectionSource.cpp
    ├── tst_ProtocolTable.cpp
    └── tst_JsonSerializer.cpp
```

### 2.1 메시지 클래스 (`src/net/messages/`)

| 파일 | 방향 | 설명 |
|---|---|---|
| `MissionRadarInfoMsg` | 제어 → 분석SW, 신호발생 | 레이다 전체 정보 (N개) |
| `MissionJammerRouteMsg` | 제어 → 분석SW | 전자전기 동선 |
| `MissionFormationMsg` | 제어 → 분석SW, 신호발생 | 아군편대 동선 (+신호발생에는 교란설정 포함) |
| `MissionFalseTargetMsg` | 제어 → 신호발생 | 단독허위표적 전체 |
| `MissionStartMsg` | 제어 → 분석SW | 임무시작 |
| `MissionStartSigGenMsg` | 제어 → 신호발생 | 임무시작 + 편대 시작위치 + 전자전기 시작위치 |
| `MissionStopMsg` | 제어 → 양쪽 | 임무종료 |
| `JammerPositionMsg` | 제어 → 신호발생 | 1초 주기 전자전기 현재위치 |
| `FormationPositionMsg` | 제어 → 신호발생 | 데이터링크 간격 주기 편대 현재위치 |
| `MainLobeNotifyMsg` | 신호발생 → 제어 | 레이다별 MainLobe 탐색 시점 + 탐지결과(연동모드 탐지 전시의 원천) |
| `HeartbeatMsg` (선택) | 양방향 | 연결/식별 확인 |
| `RawMessage` | — | 미등록 Opcode 수신 시 로그용 (hex dump) |

> 클래스명/역할은 확정, **Command Code 값과 payload 필드는 타 장비 협의 후 결정** → `docs/PROTOCOL_GUIDE.md` 절차로 추가·변경

---

## 3. 핵심 데이터 타입 (`common/Types.h`)

```cpp
struct GeoPoint { double lat = 0; double lon = 0; };      // deg, WGS84

enum class ObjectType      { Radar, Jammer, Formation, StandaloneFalseTarget };
enum class FalseTargetType : quint8 { Fixed = 0, Moving = 1, Random = 2, Noise = 3 };
enum class ScanDirection   : quint8 { Clockwise = 0, CounterClockwise = 1 };
enum class RunMode         { Standalone, Linked };
enum class SimState        { PreMission, Running, Paused, Stopped };   // 임무 전/임무 중/일시정지/중지
enum class DeviceCode : quint8 { ControlSW = 0x21, AnalysisSW = 0x92, SignalGen = 0xB1 };

using ObjectId = quint32;
```

---

## 4. 클래스 설계

### 4.1 Model 계층

```
SimObject (abstract, QObject 비상속 – 값 객체)
 ├── Radar
 ├── JammerAircraft            ── has Route
 ├── FriendlyFormation         ── has Route, JammingConfig
 └── StandaloneFalseTarget     ── (타입별) GeoPoint | Route | 없음 | GeoPoint+angle
```

| 클래스 | 주요 멤버 | 비고 |
|---|---|---|
| `SimObject` | `ObjectId id; QString name; ObjectType type() const = 0;` | id는 `IdGenerator`가 발급 |
| `RouteSegment` | `double altitudeKm; double speedMps;` | 구간 i = waypoint[i]→[i+1] |
| `Route` | `QVector<GeoPoint> waypoints; QVector<RouteSegment> segments;` `addWaypoint()`, `moveWaypoint(i, p)`, `removeWaypoint(i)`, `totalDurationSec()`, `isValid()` | waypoint 추가 시 segment 자동 추가(직전 값 복사) |
| `Radar` | `GeoPoint pos; double freqMHz, priMs, pwUs, maxRangeKm, scanPeriodSec; ScanDirection scanDir; double beamWidthDeg; int colorIndex;` | 최대 4개, colorIndex 0~3 |
| `JammerAircraft` | `Route route;` | 1개 고정 |
| `FalseTargetOffset` | `double angleDeg; double distanceKm;` | 이동방향 기준 시계방향 각도, 이격거리는 **임무 중 고정** |
| `JammingConfig` | `QVector<FalseTargetOffset> targets /*0~8*/; double noiseBandwidthMHz;` | |
| `FriendlyFormation` | `Route route; int formationSize /*1~4*/; JammingConfig jamming;` | 최대 1개 |
| `StandaloneFalseTarget` | `FalseTargetType ftType; GeoPoint pos; Route route; double detectAngleDeg;` | 타입에 따라 사용 필드 상이, `validate()` |

**`ScenarioModel : QObject`** — 전체 시나리오의 단일 진실 공급원
- 데이터: `QVector<Radar> radars; std::optional<JammerAircraft> jammer; std::optional<FriendlyFormation> formation; QVector<StandaloneFalseTarget> falseTargets; double datalinkIntervalSec;`
- API: `addRadar(GeoPoint)`, `updateRadar(const Radar&)`, `removeObject(ObjectId)`, `setJammer(...)`, `setFormation(...)`, `addFalseTarget(type)`, `moveRadar(id, GeoPoint)`, `moveWaypoint(id, idx, GeoPoint)`, `appendWaypoint(id, GeoPoint)`, `clear()`, `find(ObjectId)`
- 제약: 레이다 ≤ 4 / 편대 ≤ 1 / 허위타겟 0~8 / 편대규모 1~4 — 위반 시 `false` 반환 + `Logger` 알림
- Signals: `objectAdded(ObjectId)`, `objectChanged(ObjectId)`, `objectRemoved(ObjectId)`, `scenarioReset()`, `datalinkIntervalChanged(double)`
- 임무 중(Running/Paused)에는 `setLocked(true)` → 편집 API 거부

### 4.2 Persistence 계층

| 클래스 | 책임 |
|---|---|
| `JsonSerializer` | `static QJsonObject toJson(const Radar&)` / `fromJson(...)` 등 모델별 정적 함수. 루트에 `"schemaVersion": 1`, `"kind"` 포함 |
| `ObjectFileIO` | `saveRadars(path, list)`, `loadRadars(path)`, `saveFormation`, `loadFormation`, `saveFalseTargets`, `loadFalseTargets`, (`saveJammer`, `saveScenario` 확장) — 불러오기 시 ID는 **재발급** |

파일 확장자: `*.radar.json`, `*.formation.json`, `*.falsetarget.json`, `*.jammer.json`, (전체) `*.scenario.json`

```json
{ "schemaVersion": 1, "kind": "radar",
  "items": [ { "name": "R1", "lat": 37.5, "lon": 127.0, "freqMHz": 9400, "priMs": 1.0,
               "pwUs": 1.0, "maxRangeKm": 300, "scanPeriodSec": 10,
               "scanDir": "CW", "beamWidthDeg": 2.0 } ] }
```

### 4.3 Map 계층

**`MapProjection`** — 정거원통(Equirectangular) + 기준위도 보정
- `QPointF toScene(GeoPoint)`: `x = (lon - lon0) * k * cos(lat0)`, `y = -(lat - lat0) * k`
- `GeoPoint toGeo(QPointF)`; 기준점 `lat0=37.5, lon0=127.5` (한반도 중심)
- 거리 원/빔 형상은 `GeoUtils::destination()`으로 **지리 좌표에서 생성 후 투영**하여 왜곡 반영

**`VectorLayer`** — GeoJSON(LineString/Polygon/MultiX/Point) 파싱 → `QPainterPath` 캐시, `QPen` 스타일, `visible`, `zValue`, `minZoom`(행정구역 시군구 등은 확대 시만 표시)

**`MapLayerManager : QObject`** — 레이어 등록/표시 토글/스타일, `layerVisibilityChanged(name,bool)`
기본 레이어: 주변국 해안선·국경(어두운 회색) / 한반도 해안선(밝은 선) / 행정구역(시도·시군구) / 하천 / 산 / 군사분계선 / 위경도선

**`MapScene : QGraphicsScene`**
- 레이어를 `QGraphicsPathItem`으로 추가
- `ScenarioModel` 시그널 구독 → 오브젝트별 아이템 생성/갱신/삭제 (`QHash<ObjectId, ...>`)
- `applySnapshot(const SimSnapshot&)` → 이동체/빔/탐지마커/고스트/잡음섹터 갱신
- `resetRuntimeItems()` → 임무 후 동적 아이템 제거

**`MapView : QGraphicsView`**
- 휠 줌(커서 기준, 범위 제한), 우클릭/중클릭 드래그 팬, `centerOnKorea()`
- `mousePressEvent` → `MapEditController`로 위경도 전달
- 상태바용 `cursorGeoChanged(GeoPoint)` 시그널 (커서 위경도 표시)

**`MapEditController : QObject`** — 편집 상태머신
```
Idle ─(탐지기 추가)→ PlaceRadar ─click→ addRadar → Idle
Idle ─(편대/전자전기/이동표적 추가)→ DrawRoute ─click→ appendWaypoint (반복)
                                            ─더블클릭/Enter/우클릭→ 완료 → Idle
                                            ─Esc→ 취소(웨이포인트<2면 객체 삭제)
Idle ─(고정/잡음 표적 추가)→ PlacePoint ─click→ setPos → Idle
Idle ─ 핸들 드래그 → moveRadar / moveWaypoint (실시간 반영, 임무 중 금지)
```
- 신호: `editModeChanged(mode)`, `routeCompleted(ObjectId)` → 편집 완료 시 좌측 에디터 자동 선택

**아이템 (`map/items`)**

| 아이템 | 내용 |
|---|---|
| `RadarItem` | 중심 심볼(드래그 가능) + 최대탐지거리 원(레이다 색상, 반투명 채움) + 라벨 |
| `RadarBeamItem` | 현재 조향 방위에서 **메인로브**(빔폭 θ, 길이=최대탐지거리, 끝을 둥글게) + 양옆 **사이드로브**(작고 옅게) 형상. 형상은 `sinc²` 근사 안테나 패턴 `G(φ)`을 극좌표 샘플링해 `r(φ)=Rmax·G(φ)`로 path 생성, 방사형 그라데이션으로 빔 감쇠 표현. 형상은 빔폭 변경 시만 재계산, 매 틱은 `setRotation` 수준 갱신. 빔 지나간 자리 잔상(afterglow, 2~3개 감쇠 sweep) 옵션 |
| `RouteItem` | 폴리라인 + 구간 라벨(고도/속력) + `WaypointHandleItem` 목록, 첫 점 'S' 표시 |
| `WaypointHandleItem` | `ItemIsMovable`, 드래그 종료 시 `MapEditController`에 통지 |
| `PlatformItem` | 전자전기/편대/이동표적 심볼, 진행방향 회전, 편대규모만큼 대형 표시 |
| `FalseTargetItem` | 편대 허위타겟·고정·이동 허위표적 심볼 |
| `DetectionMarkItem` | 탐지 위치(레이다 색상), 일정 시간 후 fade (스캔주기 기준) |
| `RandomGhostItem` | 옅은 반원, 랜덤 위치 생성 → fade in/out → 제거 (탐지 판정 대상 아님) |
| `NoiseSectorItem` | 레이다별: 레이다→잡음표적 방위 ±(탐지각도/2) 섹터를 해칭/어둡게 전시 |

### 4.4 Simulation 계층

**`SimClock : QObject`**
- `QElapsedTimer` 기반 실시간 Δt × `speedFactor`(1/2/4/8) 누적 → `double simTimeSec()`
- `start()`, `pause()`, `resume()`, `stop()`, `reset()`, `setSpeedFactor(int)` (연동모드에서는 1 고정)

**`RouteKinematics`** (stateless)
- 사전계산: 구간별 길이(`GeoUtils::distance`)·소요시간(길이/속력)·누적시간
- `KinematicState stateAt(const Route&, double t)` → `{GeoPoint pos, double altKm, double headingDeg, double speedMps, bool finished}` (구간 내 대권 보간, 마지막 점 도달 시 정지)

**`RadarScanner`**
- 레이다별 상태: `bool synced; double t0; double az0;`
- `onMainLobe(radarId, t)` : `t0=t`, `az0 = bearing(radar → jammer위치(t))`
- `double beamAzimuthAt(radarId, t)` : `az0 ± 360·(t−t0)/scanPeriod` (CW:+, CCW:−), mod 360
- 단독모드: 시작 시점(및 매 스캔주기)에 엔진이 내부적으로 `onMainLobe` 생성
- 연동모드: `MainLobeNotifyMsg` 수신 전까지 빔 미전시(또는 대기 표시), 수신 시마다 재동기화

**`FalseTargetGenerator`**
- 편대 허위타겟: `destination(formationPos, heading + angle_i (시계방향), dist_i)` (i=1..N) — 이격각도·이격거리 모두 **고정값**(편대와 함께 강체처럼 이동)
- 랜덤 고스트: 각 레이다 탐지범위 내 균등 랜덤 위치, 수명 1~3초, 생성 주기 랜덤 (`QRandomGenerator`, 시드 고정 옵션)

**`IDetectionSource`** — 탐지 결과 공급 추상화 (모드별 구현 교체)
```cpp
class IDetectionSource {
public:
    virtual ~IDetectionSource() = default;
    virtual void reset() = 0;
    virtual QVector<Detection> collect(const SimContext& ctx, double simTime) = 0; // 틱마다 호출
};
```
- `Detection { quint32 radarId; QString targetKey; GeoPoint pos; double simTime; DetectionOrigin origin /*Local, External*/; }`
- 엔진은 모드에 따라 `LocalDetectionSource` / `ExternalDetectionSource` 중 하나를 주입받음

**`LocalDetectionSource`** (단독모드)
- 대상: 편대 허위타겟, 고정/이동 단독 허위표적 (랜덤 고스트는 제외)
- 조건: `거리 ≤ maxRange` AND `|방위 − 빔방위| ≤ 빔폭/2` AND `잡음섹터에 포함되지 않음`
- 잡음섹터: 잡음표적 각각에 대해 `|bearing(radar→target) − bearing(radar→noise)| ≤ detectAngle/2` 이면 해당 레이다 탐지 불가
- 결과: `QVector<Detection>` (origin = Local)

**`ExternalDetectionSource`** (연동모드)
- 연동모드의 탐지결과 = **신호발생장치로부터 수신한 MainLobe 탐지결과 메시지**
- `NetworkManager` → `MissionController`가 `MainLobeNotifyMsg`를 `push(const MainLobeNotifyMsg&)`로 큐잉 → 다음 틱 `collect()`에서 `Detection`(origin = External)으로 변환
- 같은 메시지로 `RadarScanner::onMainLobe()` 동기화 및 램프 점등 (메시지 1개 → 빔 동기화 + 램프 + 탐지 전시)
- 자체 계산 없음(잡음섹터/랜덤 고스트는 전시용으로만 유지). 메시지 필드(탐지 위치 포함 여부 등)는 ICD 협의 결과에 따름

**`SimSnapshot`** : 틱 결과 — 이동체 상태, 빔 방위, 허위타겟 위치, 탐지, 고스트, 활성 잡음섹터

**`SimulationEngine : QObject`**
- `QTimer` 틱(기본 33ms ≈ 30fps, 전시용) — 연산은 SimClock 시간 기반이므로 틱 지연과 무관
- 상태머신 `SimState` 보유: `start()`, `pause()`, `resume()`(단독모드만 허용, 연동모드 호출 시 무시+에러 로그), `stop()`, `reset()`
- 주기 이벤트 스케줄러: 시뮬레이션 시간 기준 `1s` 마다 `jammerPositionDue(state)`, `datalinkInterval` 마다 `formationPositionDue(state)` 시그널
- Signals: `snapshotReady(SimSnapshot)`, `stateChanged(SimState)`, `jammerPositionDue`, `formationPositionDue`, `mainLobeSynced(radarId)`

### 4.5 Network 계층

**메시지 프레임 (BigEndian)**
```
+--------------------------- Header (8 bytes) ---------------------------+--- Payload ---+
| Src(8) | Dst(8) | CommandCode(16) |        DataSize(32, uint)          | DataSize bytes|
+------------------ Opcode (32) ----+-------------------------------------+---------------+
```

| 클래스 | 책임 |
|---|---|
| `Opcode` | `static quint32 make(src,dst,cmd)`, `src()/dst()/cmd()` 추출, `toString()` (`0x2192 0010` 형식) — 값 자체는 보유하지 않음 |
| `MessageId` | 내부 논리 ID enum (`RadarInfo`, `JammerRoute`, …). 코드 전반은 이 ID만 사용 |
| `ProtocolTable` | `protocol.json` 로드 → `MessageId ↔ {src, dst, cmd}` 양방향 조회, 중복/누락 검증, 로드 결과 로그 |
| `MessageHeader` | `quint32 opcode; quint32 dataSize;` `encode()/decode()` |
| `ByteWriter` / `ByteReader` | `QDataStream` `BigEndian`, `setFloatingPointPrecision` 명시, 문자열은 `u16 len + UTF-8`, 언더런 검출 |
| `IMessage` | `virtual MessageId id() const; virtual void writePayload(ByteWriter&) const; virtual bool readPayload(ByteReader&); virtual QString summary() const;` |
| `MessageFactory` | `registerType<T>(MessageId)`, 수신 시 `(src, cmd)` → `ProtocolTable` → `MessageId` → 생성. 미등록은 `RawMessage` |
| `FrameDecoder` | 수신 버퍼 누적 → 헤더 8B 확보 → DataSize만큼 확보 시 프레임 방출, 최대 크기 초과 시 오류/버퍼 리셋 |
| `ClientSession : QObject` | `QTcpSocket` 래핑, 최초 수신 Opcode의 Src(또는 접속 IP 설정값)로 장치 식별, `frameReceived`, `disconnected` |
| `TcpServer : QObject` | `QTcpServer` listen(포트 설정), 세션 생성/정리 |
| `NetworkManager : QObject` | `send(DeviceCode dst, const IMessage&)`, `isConnected(DeviceCode)`, 수신 프레임 → Factory 디코드 → `messageReceived(DeviceCode, IMessage*)`; 송수신 모두 `Logger`에 기록 / Signals: `deviceConnectionChanged(DeviceCode, bool)`, `mainLobeReceived(radarId, ...)` |

- 소켓 I/O는 메인 스레드 이벤트루프(Qt 비동기)로 충분 — 데이터량이 적음. 필요 시 `NetworkManager`를 `QThread`로 이동 가능하도록 시그널/슬롯만 사용.
- 단독모드: `NetworkManager::setEnabled(false)` → 서버 close, 송신 무시.

**Command Code 확장 설계** (상세: `docs/PROTOCOL_GUIDE.md`)
- Command Code는 **타 장비 협의 후 결정** → 코드에 숫자를 하드코딩하지 않고 `protocol.json`에서 매핑
- 값 변경 = JSON 수정만(재빌드 불필요) / 새 메시지 = `MessageId` 1줄 + 메시지 클래스 1개 + Factory 등록 1줄 + JSON 1항목
- 아래 값은 **개발·모의시험용 임시 값(placeholder)**

| MessageId | 임시 Cmd | Dst | Payload 요지 (협의 대상) |

|---|---|---|---|
| RadarInfo | 0x0001 | 0x92, 0xB1 | count(u8) + [id(u32), lat, lon(f64), freq, pri, pw, range, period(f32), dir(u8), beamWidth(f32)] |
| JammerRoute | 0x0002 | 0x92 | wpCount(u16) + [lat, lon, alt, speed] |
| FormationInfo | 0x0003 | 0x92, 0xB1 | route + size(u8) + ftCount(u8) + [angle, dist] + noiseBW |
| FalseTargetInfo | 0x0004 | 0xB1 | count + [id, type(u8), 타입별 필드] |
| MissionStart | 0x0010 | 0x92, 0xB1 | (0xB1: 편대 시작위치 + 전자전기 시작위치) |
| MissionStop | 0x0012 | 0x92, 0xB1 | — |
| JammerPosition | 0x0020 | 0xB1 | simTime, lat, lon, alt, heading |
| FormationPosition | 0x0021 | 0xB1 | simTime, lat, lon, alt, heading |
| MainLobeNotify | 0x0100 | 0x21 (from 0xB1) | radarId, 탐색시점, 탐지결과 |
| Heartbeat | 0x00F0 | 양방향 | 장치코드, 상태 |

### 4.6 Mission 계층

**`MissionController : QObject`** — 버튼 동작의 단일 진입점, 활성화 규칙 관리

| 동작 | 처리 |
|---|---|
| `sendMission()` | (연동 & 양 장치 연결 시만) 분석SW: RADAR_INFO, JAMMER_ROUTE, FORMATION_INFO / 신호발생: RADAR_INFO, FORMATION_INFO, FALSE_TARGET_INFO → `missionSent=true` |
| `start()` | 시나리오 검증 → 분석SW: START / 신호발생: START(+시작위치) → `engine.start()` → `model.setLocked(true)` |
| `pause()` | **단독모드 전용** → `engine.pause()` (메시지 송신 없음); 시작 버튼 텍스트 "재개" |
| `resume()` | **단독모드 전용** → `engine.resume()` |
| `stop()` | 양 장치 STOP → `engine.stop()` |
| `refresh()` | Stopped면 즉시 초기화 / 아니면 `QMessageBox` 예·아니요 → 예: `stop()` 후 초기화 (초기화 = 엔진 reset, 동적아이템 제거, 모델 잠금 해제, 상태 PreMission; 오브젝트 설정 유지 여부는 **TBD**, 기본은 유지) |
| 엔진 `jammerPositionDue` / `formationPositionDue` | 연동모드일 때 신호발생장치로 위치 메시지 송신 |
| 수신 `MainLobeNotify` | `RadarScanner::onMainLobe()` + `ExternalDetectionSource::push()` + `MainLobeLampWidget` 점등 |

**버튼 활성화 규칙** (`updateActions()`가 상태 변경 시 일괄 계산)

| 상태 \ 버튼 | 임무전송 | 시작/재개 | 일시정지 | 중지 | 새로고침 | 배속 | 모드 변경 |
|---|---|---|---|---|---|---|---|
| 임무 전 (단독) | ✗ | ✓(검증 통과 시) | ✗ | ✗ | ✓ | ✓ | ✓ |
| 임무 전 (연동) | 두 장치 연결 시 ✓ | 임무전송 후 ✓ | ✗ | ✗ | ✓ | ✗(1배) | ✓ |
| 임무 중 (단독) | ✗ | ✗ | ✓ | ✓ | ✓(확인) | ✓ | ✗ |
| 임무 중 (연동) | ✗ | ✗ | ✗(숨김/비활성) | ✓ | ✓(확인) | ✗ | ✗ |
| 일시정지 (단독만 존재) | ✗ | ✓("재개") | ✗ | ✓ | ✓(확인) | ✓ | ✗ |
| 중지 | ✗ | ✗ | ✗ | ✗ | ✓ | ✗ | ✗ |

### 4.7 Log 계층

- `LogEntry { QDateTime time; LogType type /*Info, Error, Tx, Rx*/; QString device; quint32 opcode; quint32 size; QString content; }`
- `Logger : QObject` — `info()`, `error()`, `tx(dev, header, summary)`, `rx(...)`; `entryAdded` 시그널; 일자별 파일(`logs/yyyyMMdd.log`) 기록 옵션
- `LogTableModel` — 최대 N행(예: 10,000) 링버퍼, 유형 필터(`QSortFilterProxyModel`), 컬럼: 시각 | 유형 | 장치 | Opcode | 크기 | 내용 (Tx/Rx는 hex dump 툴팁)

### 4.8 UI 계층 (화면 레이아웃)

```
┌───────────────────────────── MapToolBar (레이어 체크 메뉴, 한반도 중심) ─────────────────────────────┐
├──────────────┬────────────────────────────────────────────────────────────────┬────────────────────┤
│ ObjectPanel  │                                                                │ ControlPanel       │
│ ┌──────────┐ │                                                                │ ┌ ModeSelector ──┐ │
│ │ActionBar │ │                         MapView                                │ │ ○단독 ○연동    │ │
│ │추가/저장/│ │                     (QGraphicsView)                            │ ├ DeviceStatus ──┤ │
│ │불러오기  │ │                                                                │ │ 분석SW  ●      │ │
│ ├──────────┤ │                                                                │ │ 신호발생 ●     │ │
│ │데이터링크│ │                                                                │ ├ MainLobe Lamp ─┤ │
│ │전송간격  │ │                                                                │ │ R1● R2● R3● R4●│ │
│ ├──────────┤ │                                                                │ ├ SimControl ────┤ │
│ │오브젝트  │ │                                                                │ │ 상태: 임무 전  │ │
│ │목록(Tree)│ │                                                                │ │ [임무전송]     │ │
│ ├──────────┤ │                                                                │ │ [시작][일시정지]│ │
│ │설정 에디터│ │                                                                │ │ [중지][새로고침]│ │
│ │(Stacked) │ │                                                                │ ├ SpeedSelector ─┤ │
│ └──────────┘ │                                                                │ │ 1x 2x 4x 8x    │ │
├──────────────┴────────────────────────────────────────────────────────────────┴────────────────────┤
│ LogPanel: 시각 | 유형 | 장치 | Opcode | 크기 | 내용                                                    │
└────────────────────────────────────────────────────────────────────────────────────────────────────┘
```
- `MainWindow`: 중앙 `MapView`, 좌/우 `QDockWidget`(또는 `QSplitter`), 하단 `LogPanel` 도크 — 크기 조절 가능, 레이아웃 `QSettings` 저장
- **ObjectActionBar**: 탐지기/아군편대/단독허위표적/(전자전기) 각각 추가·저장·불러오기
- **ObjectTreeModel**: 그룹 노드(레이다/전자전기/아군편대/단독허위표적) + 항목, 레이다는 색상 아이콘. 선택 시 `ObjectEditorStack` 전환 + 지도 하이라이트, Del/컨텍스트메뉴로 삭제
- **에디터**: `QDoubleSpinBox` 범위/단위 suffix 지정, 값 변경 즉시 `ScenarioModel::update*()`
  - `RouteSegmentTable`: 행 = 구간, 열 = 시작점/끝점(읽기전용 위경도) | 고도[km] | 속력[m/s]
  - `JammingConfigEditor`: 허위타겟개수 SpinBox(0~8) ↔ 테이블 행 수 동기화(이격각도, 초기이격거리), 잡음신호폭
  - `FalseTargetEditor`: 타입별 페이지 (고정: 위치 / 이동: 구간 테이블 / 랜덤: 안내문 / 잡음: 위치+탐지각도)
- 모든 편집 위젯은 `SimState != PreMission` 시 비활성화

---

## 5. 주요 객체 관계

```
MainWindow
 └─ AppContext (소유)
     ├─ ScenarioModel ◄──────────── editors / ObjectTreeModel / MapEditController (수정)
     │      │ signals                        
     │      ▼                                
     ├─ MapScene / MapView ◄── snapshotReady ── SimulationEngine ──┐
     │                                            ├ SimClock         │ jammer/formationPositionDue
     │                                            ├ RouteKinematics  ▼
     │                                            ├ RadarScanner ◄─ MissionController ─► NetworkManager ─► TcpServer ─► ClientSession×2
     │                                            ├ FalseTargetGenerator     ▲                 │ mainLobeReceived
     │                                            └ DetectionEvaluator       │                 │
     ├─ MissionController ◄── ControlPanel(버튼) ─┘                          └─────────────────┘
     └─ Logger ─► LogTableModel ─► LogPanel   (NetworkManager, MissionController, ScenarioModel이 기록)
```

---

## 6. 시퀀스 (요약)

**연동모드 임무 수행**
1. 분석SW / 신호발생장치 접속 → `deviceConnectionChanged` → 램프 녹색, 둘 다 연결 시 [임무전송] 활성
2. [임무전송] → 메시지 송신 (Tx 로그) → [시작] 활성
3. [시작] → START 송신 → 엔진 시작, 1초/데이터링크 주기 위치 송신
4. 신호발생장치 `MainLobeNotify` 수신 → 해당 레이다 램프 점등, 빔 동기화(전자전기 방향 기준) 후 회전 전시, 메시지의 탐지결과를 지도에 전시
5. (연동모드에서는 일시정지 불가)
6. [중지] → STOP 송신, 엔진 정지 → [새로고침] → 초기 상태

**단독모드**: 네트워크 미사용, [임무전송] 비활성, [시작] 시 내부 MainLobe 생성·자체 탐지 판정, 배속 선택 가능, 일시정지/재개 가능

---

## 7. 벡터지도 데이터 계획

| 레이어 | 후보 소스 | 비고 |
|---|---|---|
| 해안선/국경 (주변국) | Natural Earth 10m (`coastline`, `admin_0_countries`) | Public Domain |
| 한반도 해안선(정밀) | Natural Earth 10m 또는 OSM coastline 추출 | 밝은 선 |
| 행정구역(시도/시군구) | Natural Earth `admin_1`, 대한민국 행정구역 공개 데이터(통계청 SGIS/공공데이터) | 북한은 admin_1까지 |
| 하천 | Natural Earth `rivers_lake_centerlines` + `rivers_asia`, (정밀: OSM waterway) | |
| 산 | Natural Earth `geography_regions_elevation_points`, OSM `natural=peak`(주요 산만 필터) | 포인트+이름 |
| 군사분계선 | OSM 데이터 추출 또는 공개 좌표 기반 수작업 GeoJSON | 출처·라이선스 확인 필요 |
| 위경도선 | 런타임 생성 (`GraticuleLayer`) | 데이터 불필요 |

- 영역: 대략 **N 30°~45°, E 118°~135°** (한반도 + 중국 동북부·산둥, 일본 서부, 러시아 연해주 일부)
- `tools/map_prep`에서 bbox clip + 단순화(mapshaper) → 각 파일 수 MB 이하로 경량화 후 `resources/map`에 포함(오프라인 동작 필수)
- 라이선스(OSM: ODbL 표기 필요) README에 명시

---

## 8. 빌드 / 환경 구성 계획 (개발 착수 시 수행)

1. **Macbook**: Homebrew `qt@6`(또는 Qt Online Installer 6.8 LTS), `cmake`, `ninja` 설치 → 지정 경로에 프로젝트 생성
2. `CMakeLists.txt`: `find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)`, `qt_standard_project_setup()`, `CMAKE_AUTOMOC/AUTORCC/AUTOUIC ON`, 경고 레벨 설정
3. **Windows (OMEN / ThinkPad)**: Qt 6.x MSVC2022 64-bit + Visual Studio 2022 → `cmake --preset windows-msvc-release` → `windeployqt`로 배포 폴더 생성
4. `.gitignore` (build/, *.user, .DS_Store), GitHub Actions(선택): macOS/Windows 빌드 + Qt Test 실행
5. 줄바꿈/인코딩: 소스 UTF-8 (MSVC `/utf-8` 옵션), `.gitattributes`로 LF 통일

---

## 9. 개발 단계 (마일스톤 제안)

| 단계 | 범위 | 완료 기준 |
|---|---|---|
| M1 | 프로젝트 골격, MainWindow 레이아웃, Logger/LogPanel | 빈 화면 4분할 + 로그 출력 |
| M2 | 지도: 투영, GeoJSON 레이어, 줌/팬, 레이어 토글, 한반도 중심 | 지도 전시 |
| M3 | Model + 오브젝트 패널 + 지도 편집(배치/동선/드래그) + 저장/불러오기 | 시나리오 작성 가능 |
| M4 | 시뮬레이션 엔진(단독모드): 이동, 빔, 허위타겟, 탐지, 랜덤, 잡음섹터, 배속 | 단독 시연 가능 |
| M5 | 네트워크: 서버, 프레임, 메시지, 연동 상태, 임무전송/제어/주기송신, MainLobe 수신 | 테스트 클라이언트와 연동 |
| M6 | 테스트용 모의 클라이언트(분석SW/신호발생장치 에뮬레이터, `tools/mock_client`), Windows 빌드/배포 점검 | ThinkPad 실행 확인 |

---

## 10. 미확정 사항 (TBD — 개발 중 확인 필요)

1. 메시지 Command Code 및 payload 필드 정의(ICD) — 타 장비 협의 후 `protocol.json` + 메시지 클래스에 반영
2. 실수형 인코딩(f32 vs f64 vs 고정소수점 정수)
3. 장치 식별 방식: 접속 IP/포트 설정 vs 첫 메시지 Src 코드
4. MainLobe 탐지결과 메시지에 탐지 위치(위경도/거리·방위) 포함 형태
5. ~~전자전기 오브젝트의 저장/불러오기 및 데이터링크 전송간격 저장 위치~~ → `*.jammer.json`, `*.scenario.json`에 포함 (2026-11-22)
6. 새로고침 시 설정된 오브젝트 유지 여부
7. MainLobe 수신 램프 점등 시간/방식(펄스 vs 래치)
8. 동선 종료 이후 이동체 처리(정지/소멸/루프)

### 결정 사항 이력
| 일자 | 결정 |
|---|---|
| 2026-11-20 | Command Code는 타 장비 협의 후 결정 → 외부 매핑(`protocol.json`) 기반 확장 설계 |
| 2026-11-20 | 일시정지/재개는 **단독모드 전용** (PAUSE 메시지 삭제) |
| 2026-11-20 | 아군편대 허위타겟 이격거리는 **고정** |
| 2026-11-20 | 연동모드 탐지결과 = 신호발생장치로부터 **수신한 MainLobe 탐지결과 메시지** |
| 2026-11-21 | 지도 데이터는 Natural Earth 10m 단일 출처(Public Domain), Python 표준 라이브러리 전처리(`tools/map_prep`). 군사분계선·DMZ도 Natural Earth 사용, 한반도 주요 산은 직접 입력. 시군구(`kr_admin2`)는 보류 |
| 2026-11-22 | M3 구현 세부: `StandaloneFalseTarget.posSet`(위치 미지정 상태), `ScenarioModel::renameObject`, 전자전기 저장/불러오기 및 `*.scenario.json`(데이터링크 간격 포함) 구현 → TBD 5 해소. 항목별 불러오기는 기존 항목 **교체** |
| 2026-11-22 | 빌드 구조: `ctrlsw_core`(비UI) / `ctrlsw_gui`(Widgets) 정적 라이브러리 + `CtrlSW` 실행파일. 위젯 테스트(`tst_MapEdit`)는 offscreen 실행 |
| 2026-11-21 | Scene 좌표 1 단위 ≈ 1 km (`k = 111.32`), 지도 배율 단위 px/km. 축소(0.6 px/km 미만) 시 단순화 경로 사용 |
