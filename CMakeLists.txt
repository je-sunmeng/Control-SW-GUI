cmake_minimum_required(VERSION 3.21)

project(CtrlSW
    VERSION 0.4.0
    DESCRIPTION "Stand Off Jammer - Radar demonstration control SW"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 6.8 LTS 권장, 최소 6.4
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(CTRLSW_BUILD_TESTS "Build unit tests" ON)
option(CTRLSW_BUILD_TOOLS "Build tools (mock client)" ON)

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Windows 실행파일에 버전 정보(파일 속성) 추가. 설명 문자열은 ASCII (rc.exe 코드페이지 문제 회피)
function(ctrlsw_add_version_info target name description)
    if(WIN32)
        set(RC_NAME "${name}")
        set(RC_DESCRIPTION "${description}")
        set(rc "${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc")
        configure_file(${PROJECT_SOURCE_DIR}/cmake/version.rc.in ${rc} @ONLY)
        target_sources(${target} PRIVATE ${rc})
    endif()
endfunction()

add_subdirectory(src)

if(CTRLSW_BUILD_TOOLS OR CTRLSW_BUILD_TESTS)
    add_subdirectory(tools/mock_client)
endif()

if(CTRLSW_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
