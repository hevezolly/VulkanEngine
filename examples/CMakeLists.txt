
file(GLOB EXAMPLE_FILES *.cpp)

foreach (tgt_full ${EXAMPLE_FILES})

    get_filename_component(tgt ${tgt_full} NAME_WE)
    
    add_executable(${tgt} ${tgt}.cpp)
    target_include_directories(${tgt} PRIVATE .)

    target_link_libraries(${tgt} PRIVATE VulkanEngine)

    set_target_properties(${tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples/${tgt}
    )

    foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${cfg} OUTPUTCONFIG_UP)
        set_target_properties(${tgt} PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${CMAKE_BINARY_DIR}/${cfg}/bin/examples/${tgt})
    endforeach()

    if (WIN32 AND BUILD_SHARED_LIBS)
        add_custom_command(TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${tgt}>
            $<TARGET_FILE_DIR:${tgt}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endforeach()