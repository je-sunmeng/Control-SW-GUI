# ---------------------------------------------------------------------------
# ctrlsw_core : UI(Widgets)에 의존하지 않는 계층 (common, log, 이후 model/sim/net/persistence)
#               → 단위 테스트에서 직접 링크
# ---------------------------------------------------------------------------
qt_add_library(ctrlsw_core STATIC
    common/Constants.h
    common/Types.h
    common/Types.cpp
    common/GeoUtils.h
    common/GeoUtils.cpp

    log/LogEntry.h
    log/LogEntry.cpp
    log/Logger.h
    log/Logger.cpp
    log/LogTableModel.h
    log/LogTableModel.cpp
    log/LogFilterProxyModel.h
    log/LogFilterProxyModel.cpp

    map/MapProjection.h
    map/MapProjection.cpp
    map/VectorLayer.h
    map/VectorLayer.cpp
    map/MapLayerManager.h
    map/MapLayerManager.cpp
)
target_include_directories(ctrlsw_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ctrlsw_core PUBLIC Qt6::Core Qt6::Gui)
target_compile_definitions(ctrlsw_core PUBLIC CTRLSW_VERSION="${PROJECT_VERSION}")

# 벡터지도 데이터 (tools/map_prep 으로 생성). 테스트에서도 쓰도록 core 에 포함
file(GLOB CTRLSW_MAP_FILES CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/resources/map/*.geojson)
qt_add_resources(ctrlsw_core "mapdata"
    PREFIX "/"
    BASE ${PROJECT_SOURCE_DIR}/resources
    FILES ${CTRLSW_MAP_FILES}
)

# ---------------------------------------------------------------------------
# CtrlSW : GUI 실행 파일 (app, ui)
# ---------------------------------------------------------------------------
qt_add_executable(CtrlSW
    main.cpp

    app/AppContext.h
    app/AppContext.cpp
    app/MainWindow.h
    app/MainWindow.cpp

    ui/MapToolBar.h
    ui/MapToolBar.cpp
    map/GraticuleLayer.h
    map/GraticuleLayer.cpp
    map/MapScene.h
    map/MapScene.cpp
    map/MapView.h
    map/MapView.cpp
    map/items/VectorLayerItem.h
    map/items/VectorLayerItem.cpp

    ui/ObjectPanel.h
    ui/ObjectPanel.cpp
    ui/ControlPanel.h
    ui/ControlPanel.cpp
    ui/LogPanel.h
    ui/LogPanel.cpp

    ${PROJECT_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(CtrlSW PRIVATE ctrlsw_core Qt6::Widgets Qt6::Network)

set_target_properties(CtrlSW PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "CtrlSW"
    MACOSX_BUNDLE_GUI_IDENTIFIER "kr.ctrlsw.radar"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)
