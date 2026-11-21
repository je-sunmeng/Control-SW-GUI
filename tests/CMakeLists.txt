# Qt Test 기반 단위 테스트. 테스트 1개 = 실행파일 1개.
function(ctrlsw_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ctrlsw_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

ctrlsw_add_test(tst_GeoUtils)
ctrlsw_add_test(tst_Logger)
