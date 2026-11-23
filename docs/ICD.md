# 메시지 인터페이스 정의서 (ICD) — 초안

> 상태: **협의용 초안 v0.1-draft** (2026-11-22). 아래 Command Code·payload 는 제어SW 에 **임시로 구현된 값**이며,
> 분석SW(0x92)·신호발생장치(0xB1) 와 협의 후 확정한다. 변경 절차는 [PROTOCOL_GUIDE.md](PROTOCOL_GUIDE.md) 4장.

## 1. 공통

| 항목 | 내용 |
|---|---|
| 전송 | TCP. **제어SW 가 서버** (기본 포트 50021, `--port` 로 변경), 분석SW·신호발생장치가 클라이언트로 접속 |
| 바이트 순서 | BigEndian (네트워크 순서) |
| 프레임 | 헤더 8B + payload. 헤더 = Opcode(u32) + DataSize(u32, payload 바이트 수) |
| Opcode | Src(u8) \| Dst(u8) \| CommandCode(u16) — 예: `0x2192 0010` = 제어SW→분석SW, Cmd 0x0010 |
| 장치 코드 | 제어SW 0x21 / 분석SW 0x92 / 신호발생장치 0xB1 |
| 장치 식별 | 접속 후 **첫 프레임의 Src** 로 식별 → 클라이언트는 접속 직후 Heartbeat 1회 송신 권장 (TBD-3) |
| 실수 | 위경도 f64, 그 외 f32 (IEEE-754) — TBD-2 협의 대상 |
| 시간 | 시뮬레이션 경과 시간(초, f64). 임무 시작 = 0 |
| 최대 DataSize | 16 MiB 초과 시 수신 측에서 스트림 오류로 처리 |
| 미등록 Opcode | 수신 측은 hex dump 로그만 남기고 무시 |

### 1.1 동선 공통 형식 `Route`

| # | 필드 | 타입 | 단위 | 설명 |
|---|---|---|---|---|
| 1 | wpCount | u16 | | 웨이포인트 수 N |
| 2.. | lat, lon, altKm, speedMps | f64, f64, f32, f32 | deg, deg, km, m/s | × N. 점 i 의 고도/속력 = 구간 i(i→i+1). 마지막 점은 마지막 구간 값 |

## 2. 메시지 목록

| MessageId | Cmd(임시) | 방향 | 시점 |
|---|---|---|---|
| RadarInfo | 0x0001 | 제어 → 분석SW, 신호발생 | [임무전송] |
| JammerRoute | 0x0002 | 제어 → 분석SW | [임무전송] |
| FormationInfo | 0x0003 | 제어 → 분석SW, 신호발생 | [임무전송] (편대 있을 때) |
| FalseTargetInfo | 0x0004 | 제어 → 신호발생 | [임무전송] |
| MissionStart | 0x0010 | 제어 → 분석SW, 신호발생 | [시작] |
| MissionStop | 0x0012 | 제어 → 분석SW, 신호발생 | [중지] / 임무 중 [새로고침] |
| JammerPosition | 0x0020 | 제어 → 신호발생 | 임무 중 1 s 주기 (시뮬레이션 시간) |
| FormationPosition | 0x0021 | 제어 → 신호발생 | 임무 중 데이터링크 전송간격 주기 |
| MainLobeNotify | 0x0100 | 신호발생 → 제어 | 레이다 MainLobe 탐색 시 |
| Heartbeat | 0x00F0 | 양방향 | 접속 직후 (주기 TBD) |

## 3. 메시지 상세

### RadarInfo (0x21 → 0x92 / 0xB1, Cmd 0x0001)

| # | 필드 | 타입 | 단위 | 설명 |
|---|---|---|---|---|
| 1 | count | u8 | | 레이다 수 (≤4) |
| 2.. | id | u32 | | 레이다 ID (MainLobeNotify.radarId 와 동일 체계) |
| | lat, lon | f64, f64 | deg | 위치 |
| | freqMHz, priMs, pwUs | f32 ×3 | MHz, ms, µs | |
| | maxRangeKm, scanPeriodSec | f32 ×2 | km, s | |
| | scanDir | u8 | | 0 = 시계(CW), 1 = 반시계(CCW) |
| | beamWidthDeg | f32 | deg | |

샘플 (레이다 1기: id 1, 37.5N 127.0E, 9400 MHz, PRI 1, PW 1, 300 km, 10 s, CCW, 2°):
```
21 92 00 01 00 00 00 2E   01 00 00 00 01 40 42 C0 00 00 00 00 00 40 5F C0 00 00 00 00 00
46 12 E0 00 3F 80 00 00 3F 80 00 00 43 96 00 00 41 20 00 00 01 40 00 00 00
```

### JammerRoute (0x21 → 0x92, Cmd 0x0002)
`Route` (1.1). 전자전기 동선은 **루프**: 마지막 점 = 시작점이 되도록 시작점을 한 번 더 넣어 송신 (점 N+1 개), 임무 중 반복 비행

### FormationInfo (0x21 → 0x92 / 0xB1, Cmd 0x0003)

| # | 필드 | 타입 | 설명 |
|---|---|---|---|
| 1 | route | Route | |
| 2 | formationSize | u8 | 1~4 |
| 3 | hasJamming | u8 | 분석SW 행 0, 신호발생 행 1 |
| 4 | ftCount | u8 | (hasJamming=1) 허위타겟 수 0~8 |
| 5.. | angleDeg, distanceKm | f32, f32 | × ftCount. 편대 이동방향 기준 시계방향 각도, 이격거리(임무 중 고정) |
| 6 | noiseBandwidthMHz | f32 | (hasJamming=1) 잡음신호폭 |

### FalseTargetInfo (0x21 → 0xB1, Cmd 0x0004)

| # | 필드 | 타입 | 설명 |
|---|---|---|---|
| 1 | count | u8 | |
| 2.. | id | u32 | |
| | type | u8 | 0 고정 / 1 이동 / 2 랜덤 / 3 잡음 |
| | (고정) lat, lon | f64, f64 | |
| | (이동) route | Route | |
| | (랜덤) — | | 필드 없음 |
| | (잡음) lat, lon, detectAngleDeg | f64, f64, f32 | 레이다→잡음표적 방위 ±각/2 섹터 탐지불가 |

### MissionStart (Cmd 0x0010)
- 0x21 → 0x92: payload 없음 (DataSize 0)
- 0x21 → 0xB1: `formationLat, formationLon, jammerLat, jammerLon` (f64 ×4, 32 B). 편대 없으면 편대 좌표 0,0 (협의 필요)

### MissionStop (Cmd 0x0012)
payload 없음

### JammerPosition (Cmd 0x0020) / FormationPosition (Cmd 0x0021) — 0x21 → 0xB1

| # | 필드 | 타입 | 단위 |
|---|---|---|---|
| 1 | simTime | f64 | s |
| 2 | lat, lon | f64, f64 | deg |
| 3 | altKm | f32 | km |
| 4 | headingDeg | f32 | deg (북 0, 시계방향) |

### MainLobeNotify (0xB1 → 0x21, Cmd 0x0100)

| # | 필드 | 타입 | 단위 | 설명 |
|---|---|---|---|---|
| 1 | radarId | u32 | | RadarInfo 의 id |
| 2 | searchTime | f64 | s | MainLobe 탐색 시점 |
| 3 | detCount | u8 | | 탐지 수 |
| 4.. | lat, lon | f64, f64 | deg | × detCount, 탐지 위치 (TBD-4: 거리·방위 표현 여부) |

제어SW 동작: 수신 즉시 해당 레이다 빔을 "레이다→현재 전자전기 방위" 로 재동기화하고 MainLobe 램프를 점등, 탐지 위치를 지도에 전시.

### Heartbeat (Cmd 0x00F0, 양방향)

| # | 필드 | 타입 | 설명 |
|---|---|---|---|
| 1 | deviceCode | u8 | 송신 장치 코드 |
| 2 | status | u8 | 0 = 정상 |

## 4. 협의 필요 항목
1. Command Code 값 체계 (송신/수신 대역 구분 여부)
2. 실수 타입 (f32/f64/스케일 정수) — 현재 위경도 f64, 나머지 f32
3. 장치 식별: 첫 프레임 Src vs 접속 IP 설정
4. MainLobeNotify 탐지 위치 표현 (위경도 vs 거리·방위), 탐지 없음 시 메시지 송신 여부
5. 응답(ACK) 필요 여부, Heartbeat 주기·타임아웃
6. 편대 없는 시나리오의 MissionStart(신호발생) 편대 좌표 표현
