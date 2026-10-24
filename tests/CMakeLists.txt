function(add_avestan_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE avestan-core Qt6::Test)
    qt_add_resources(${name} "${name}_assets"
        PREFIX "/"
        BASE "${CMAKE_SOURCE_DIR}"
        FILES ${AVESTAN_ASSETS}
    )
    add_test(NAME ${name} COMMAND ${name} -platform offscreen)
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_avestan_test(test_document)
add_avestan_test(test_config)
add_avestan_test(test_keyboard)
add_avestan_test(test_editor)
