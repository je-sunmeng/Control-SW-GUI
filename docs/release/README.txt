전자전기-레이다 시연 제어 SW (CtrlSW) — Windows x64 배포본
==========================================================

설치 불필요: 압축을 풀고 CtrlSW.exe 를 실행합니다. (Qt 런타임·VC++ 런타임 포함)

[단독모드 시연]
  1) CtrlSW.exe 실행
  2) 좌측 [시나리오 불러오기] → samples\demo.scenario.json
  3) 우측 [시작] (배속 1x/2x/4x/8x, 일시정지 가능)
  명령줄로 바로 시연:  CtrlSW.exe --scenario samples\demo.scenario.json --speed 4 --start

[연동모드 시험 — 모의 장치 사용]
  1) CtrlSW.exe --scenario samples\demo.scenario.json --linked
     (Windows 방화벽 허용 창이 뜨면 "허용")
  2) 다른 명령 창에서:  ctrlsw_mock.exe --device both
     (다른 PC 에서 접속: ctrlsw_mock.exe --host <제어SW PC IP> --device both)
  3) CtrlSW 에서 [임무전송] → [시작]
  기본 포트 50021 (변경: CtrlSW.exe --port <번호>, ctrlsw_mock.exe --port <번호>)

[Command Code 변경]
  protocol.json.example 을 protocol.json 으로 이름을 바꾸고 값을 수정하면
  재빌드 없이 적용됩니다 (프로그램 재시작). 형식: docs/PROTOCOL_GUIDE.md

[로그]
  %LOCALAPPDATA%\CtrlSW\CtrlSW\logs\yyyyMMdd.log

주의: 메시지 Command Code / payload 는 타 장비 협의 전 임시안입니다 (docs/ICD.md).
지도 데이터: Natural Earth (Public Domain)
