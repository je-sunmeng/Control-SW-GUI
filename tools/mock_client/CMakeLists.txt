# 연동 시험용 모의 클라이언트 (분석SW / 신호발생장치 에뮬레이터)
qt_add_library(ctrlsw_mock STATIC
    MockDevice.h
    MockDevice.cpp
)
target_include_directories(ctrlsw_mock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ctrlsw_mock PUBLIC ctrlsw_core)

qt_add_executable(ctrlsw_mock_client main.cpp)
target_link_libraries(ctrlsw_mock_client PRIVATE ctrlsw_mock)
set_target_properties(ctrlsw_mock_client PROPERTIES
    OUTPUT_NAME ctrlsw_mock
    MACOSX_BUNDLE OFF
    WIN32_EXECUTABLE OFF)
ctrlsw_add_version_info(ctrlsw_mock_client ctrlsw_mock "CtrlSW mock client - AnalysisSW / SignalGen emulator")
