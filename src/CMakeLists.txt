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
    common/IdGenerator.h
    common/ColorPalette.h
    common/ColorPalette.cpp

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

    model/SimObject.h
    model/SimObject.cpp
    model/Route.h
    model/Route.cpp
    model/Radar.h
    model/Radar.cpp
    model/JammerAircraft.h
    model/JammingConfig.h
    model/JammingConfig.cpp
    model/FriendlyFormation.h
    model/StandaloneFalseTarget.h
    model/StandaloneFalseTarget.cpp
    model/ScenarioModel.h
    model/ScenarioModel.cpp

    persistence/JsonSerializer.h
    persistence/JsonSerializer.cpp
    persistence/ObjectFileIO.h
    persistence/ObjectFileIO.cpp
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
# ctrlsw_gui : Widgets 계층 (app, map 전시/편집, ui) → 실행 파일 + 위젯 테스트에서 링크
# ---------------------------------------------------------------------------
qt_add_library(ctrlsw_gui STATIC
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
    map/MapEditController.h
    map/MapEditController.cpp
    map/items/VectorLayerItem.h
    map/items/VectorLayerItem.cpp
    map/items/MapObjectItem.h
    map/items/WaypointHandleItem.h
    map/items/WaypointHandleItem.cpp
    map/items/RadarItem.h
    map/items/RadarItem.cpp
    map/items/RouteItem.h
    map/items/RouteItem.cpp
    map/items/FalseTargetItem.h
    map/items/FalseTargetItem.cpp

    ui/ObjectPanel.h
    ui/ObjectPanel.cpp
    ui/ObjectActionBar.h
    ui/ObjectActionBar.cpp
    ui/ObjectTreeModel.h
    ui/ObjectTreeModel.cpp
    ui/ObjectListView.h
    ui/ObjectListView.cpp
    ui/editors/EditorUtil.h
    ui/editors/ObjectEditorBase.h
    ui/editors/ObjectEditorBase.cpp
    ui/editors/ObjectEditorStack.h
    ui/editors/ObjectEditorStack.cpp
    ui/editors/RouteSegmentTable.h
    ui/editors/RouteSegmentTable.cpp
    ui/editors/RadarEditor.h
    ui/editors/RadarEditor.cpp
    ui/editors/JammerEditor.h
    ui/editors/JammerEditor.cpp
    ui/editors/FormationEditor.h
    ui/editors/FormationEditor.cpp
    ui/editors/JammingConfigEditor.h
    ui/editors/JammingConfigEditor.cpp
    ui/editors/FalseTargetEditor.h
    ui/editors/FalseTargetEditor.cpp
    ui/dialogs/FalseTargetTypeDialog.h
    ui/dialogs/FalseTargetTypeDialog.cpp
    ui/ControlPanel.h
    ui/ControlPanel.cpp
    ui/LogPanel.h
    ui/LogPanel.cpp
)
target_link_libraries(ctrlsw_gui PUBLIC ctrlsw_core Qt6::Widgets Qt6::Network)

# ---------------------------------------------------------------------------
# CtrlSW : GUI 실행 파일
# ---------------------------------------------------------------------------
qt_add_executable(CtrlSW
    main.cpp
    ${PROJECT_SOURCE_DIR}/resources/resources.qrc
)
target_link_libraries(CtrlSW PRIVATE ctrlsw_gui)

set_target_properties(CtrlSW PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "CtrlSW"
    MACOSX_BUNDLE_GUI_IDENTIFIER "kr.ctrlsw.radar"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)
