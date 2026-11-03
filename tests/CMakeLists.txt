function (tfs_add_test_executable target label)
    file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${label}/test_*.cpp)

    add_executable(${target} ${CMAKE_CURRENT_SOURCE_DIR}/${label}/main.cpp ${sources})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE tfslib ZLIB::ZLIB OpenSSL::Crypto)

    foreach (source ${sources})
        get_filename_component(test_name ${source} NAME_WE)
        add_test(NAME ${test_name} COMMAND ${target} --run_test=${test_name} --report_level=short --color_output=no)
        set_tests_properties(${test_name} PROPERTIES LABELS ${label} ${ARGN})
    endforeach ()
endfunction ()

tfs_add_test_executable(tfs_tests unit)
tfs_add_test_executable(tfs_integration_tests integration SKIP_RETURN_CODE 77 RESOURCE_LOCK database)

target_sources(tfs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lua/runner.cpp)
target_compile_definitions(tfs_tests PRIVATE TFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

file(GLOB lua_tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua/test_*.lua)
foreach (lua_test ${lua_tests})
    get_filename_component(test_name ${lua_test} NAME_WE)
    string(REGEX REPLACE "^test_" "lua_" test_name ${test_name})
    add_test(NAME ${test_name} COMMAND tfs_tests --run_test=lua --report_level=short --color_output=no -- ${lua_test})
    set_tests_properties(${test_name} PROPERTIES LABELS lua)
endforeach ()
