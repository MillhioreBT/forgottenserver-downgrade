file(GLOB TFS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_*.cpp)

add_executable(tfs_tests main.cpp ${TFS_TEST_SOURCES})
target_include_directories(tfs_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tfs_tests PRIVATE tfslib ZLIB::ZLIB OpenSSL::Crypto)

foreach (test_source ${TFS_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_test(NAME ${test_name} COMMAND tfs_tests --run_test=${test_name} --report_level=short --color_output=no)
endforeach ()
