# Command Code / 메시지 확장 설계 및 작성 가이드

> Command Code와 payload는 **타 장비(분석SW 0x92, 신호발생장치 0xB1)와 협의 후 결정**합니다.
> 이 문서는 값이 정해지지 않은 상태에서도 개발을 진행하고, 협의 결과를 **최소 수정**으로 반영하기 위한 구조와 절차를 정의합니다.

---

## 1. 설계 개념

```
 코드 내부 (고정)                         외부 설정 (협의 결과에 따라 변경)
┌──────────────────────┐   ProtocolTable   ┌──────────────────────────────┐
│ MessageId::RadarInfo │ ◄──────────────► │ protocol.json                 │
│ RadarInfoMsg 클래스  │                   │ "RadarInfo": src/dst/cmd 값   │
└──────────────────────┘                   └──────────────────────────────┘
```

| 구분 | 위치 | 언제 바뀌나 |
|---|---|---|
| **MessageId** (논리 ID) | `src/net/MessageId.h` | 메시지 종류가 **새로 생길 때만** |
| **Command Code 값** | `resources/protocol/protocol.json` | 협의로 **번호가 정해지거나 바뀔 때** (재빌드 불필요) |
| **Payload 구조** | `src/net/messages/*Msg.cpp` | 협의로 **필드가 정해지거나 바뀔 때** |

- 코드 어디에서도 `0x0010` 같은 숫자를 직접 쓰지 않습니다. 항상 `MessageId`로 송신/분기합니다.
- 같은 의미의 메시지라도 목적지별 Command Code가 다를 수 있으므로 매핑은 **(MessageId, Dst) 단위**입니다.
- 수신 시에는 `(Src, Cmd)` → `MessageId` 역조회 후 `MessageFactory`로 객체 생성. 매핑에 없으면 `RawMessage`로 hex dump 로그만 남깁니다.

---

## 2. protocol.json 형식

```json
{
  "protocolVersion": "0.1-draft",
  "devices": { "ControlSW": "0x21", "AnalysisSW": "0x92", "SignalGen": "0xB1" },
  "messages": [
    { "id": "RadarInfo",        "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0001" },
    { "id": "RadarInfo",        "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0001" },
    { "id": "JammerRoute",      "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0002" },
    { "id": "FormationInfo",    "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0003" },
    { "id": "FormationInfo",    "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0003" },
    { "id": "FalseTargetInfo",  "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0004" },
    { "id": "MissionStart",     "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0010" },
    { "id": "MissionStart",     "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0010" },
    { "id": "MissionStop",      "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0012" },
    { "id": "MissionStop",      "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0012" },
    { "id": "JammerPosition",   "src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0020" },
    { "id": "FormationPosition","src": "ControlSW", "dst": "SignalGen",  "cmd": "0x0021" },
    { "id": "MainLobeNotify",   "src": "SignalGen", "dst": "ControlSW",  "cmd": "0x0100" }
  ]
}
```
(값은 모의시험용 임시 값)

**로드 규칙 (`ProtocolTable::load`)**
1. 실행파일 옆 `protocol.json`이 있으면 우선 사용, 없으면 리소스(`:/protocol/protocol.json`) 기본값 사용
2. 검증: 알 수 없는 `id`, 같은 `(src,dst,cmd)` 중복, 코드에 등록된 메시지인데 매핑 누락 → 에러 로그 (프로그램은 계속 실행, 해당 메시지만 송신 불가)
3. 시작 시 로그에 `protocolVersion`과 로드된 매핑 개수 출력 → 장비 간 버전 불일치 확인용

---

## 3. 메시지 클래스 작성 규칙

```cpp
// src/net/messages/RadarInfoMsg.h
class RadarInfoMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::RadarInfo; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;          // 로그 '내용' 컬럼에 표시

    QVector<Radar> radars;                      // 메시지 데이터 (모델 타입 재사용 가능)
};

// src/net/messages/RadarInfoMsg.cpp
void RadarInfoMsg::writePayload(ByteWriter& w) const {
    w.u8(quint8(radars.size()));
    for (const auto& r : radars) {
        w.u32(r.id);
        w.f64(r.pos.lat);  w.f64(r.pos.lon);
        w.f32(r.freqMHz);  w.f32(r.priMs);  w.f32(r.pwUs);
        w.f32(r.maxRangeKm); w.f32(r.scanPeriodSec);
        w.u8(quint8(r.scanDir)); w.f32(r.beamWidthDeg);
    }
}
bool RadarInfoMsg::readPayload(ByteReader& r) { /* write와 같은 순서 */ return r.ok(); }
```

| 규칙 | 내용 |
|---|---|
| 바이트 순서 | `ByteWriter/Reader`만 사용 (BigEndian 보장). `memcpy`/구조체 캐스팅 금지 |
| 타입 | 정수 `u8/u16/u32/i32`, 실수 `f32/f64` 명시 (협의된 타입 그대로) |
| 순서 | `writePayload`와 `readPayload`의 필드 순서를 완전히 동일하게, ICD 표 순서와 일치 |
| 반복 필드 | 개수 필드를 먼저 쓰고 항목을 반복 (`count + items`) |
| 문자열 | `u16 길이 + UTF-8` (사용 여부는 협의) |
| DataSize | 직접 계산 금지 — `NetworkManager`가 payload 직렬화 후 자동 기입 |
| 수신 검증 | `readPayload`는 언더런/남은 바이트 존재 시 `false` → 에러 로그 |
| summary() | `"radars=3 [R1 9400MHz ...]"` 처럼 사람이 읽을 요약 |

---

## 4. 작성 절차

### 4.1 협의로 Command Code 값만 정해지거나 바뀐 경우
1. `protocol.json`의 `cmd` 값 수정, `protocolVersion` 갱신
2. `docs/ICD.md` 표 갱신
3. 프로그램 재시작 (재빌드 불필요) → 로그에서 매핑 로드 확인

### 4.2 기존 메시지의 payload 필드가 바뀐 경우
1. `docs/ICD.md`에 필드 표(순서/타입/단위/범위) 갱신
2. 해당 `*Msg.cpp`의 `writePayload`/`readPayload` 수정
3. `tests/tst_MessageCodec.cpp`에 **기대 바이트열** 테스트 갱신 (타 장비에서 받은 샘플 hex가 있으면 그대로 사용)

### 4.3 새 메시지를 추가하는 경우 (예: `RadarStatus` 수신)
1. `MessageId.h`에 `RadarStatus` 추가 (+ `toString` 테이블)
2. `src/net/messages/RadarStatusMsg.h/.cpp` 작성 (3장 규칙)
3. `MessageRegistry.cpp`에 등록 1줄: `factory.registerType<RadarStatusMsg>(MessageId::RadarStatus);`
4. `protocol.json`에 매핑 항목 추가
5. 송신이면 `MissionController`에서 `network.send(dst, msg)` 호출 / 수신이면 `NetworkManager::messageReceived` 핸들러에 `MessageId` 분기 추가
6. `tst_MessageCodec`에 왕복(write→read) 테스트 추가, `docs/ICD.md` 갱신

### 4.4 체크리스트
- [ ] 코드에 Command Code 숫자를 직접 쓰지 않았다
- [ ] write/read 필드 순서·타입이 ICD와 같다
- [ ] 왕복 테스트와 기대 바이트열 테스트를 추가했다
- [ ] protocol.json에 모든 (src, dst) 조합을 넣었다
- [ ] 로그에서 Tx/Rx Opcode·크기·요약이 정상 표시된다

---

## 5. ICD.md 작성 양식 (협의용)

타 장비와 협의할 때 아래 양식으로 메시지별 표를 만들어 공유합니다.

```markdown
### MissionStart (제어SW 0x21 → 신호발생장치 0xB1)
- Opcode: 0x21B1 ____ (Command Code: 협의)
- 송신 시점: [시작] 버튼 클릭 시 1회
- DataSize: 고정 __ bytes

| # | 필드 | 타입 | 단위 | 범위/설명 |
|---|---|---|---|---|
| 1 | formationLat | f64 | deg | 아군편대 시작 위도 |
| 2 | formationLon | f64 | deg | 아군편대 시작 경도 |
| 3 | jammerLat | f64 | deg | 전자전기 시작 위도 |
| 4 | jammerLon | f64 | deg | 전자전기 시작 경도 |

샘플(hex): 21 B1 00 10 00 00 00 20 40 42 C0 00 ...
```

협의 시 확인할 항목: Command Code 값 체계(송신/수신 대역 구분 여부), 실수 타입(f32/f64/정수 스케일), 시간 표현(시뮬레이션 초 vs 절대시각), ID 체계, 응답(ACK) 필요 여부, 연결 확인(Heartbeat) 주기, MainLobe 탐지결과 메시지의 탐지 위치 표현.
