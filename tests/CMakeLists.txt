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
