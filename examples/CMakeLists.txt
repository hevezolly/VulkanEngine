
file(GLOB EXAMPLE_FILES *.cpp)
set(SOURCE_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(DESTINATION_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/resources")

if(EXISTS "${SOURCE_RESOURCES}")
    file(COPY "${SOURCE_RESOURCES}/" DESTINATION "${DESTINATION_RESOURCES}")
endif()

foreach (tgt_full ${EXAMPLE_FILES})

    get_filename_component(tgt ${tgt_full} NAME_WE)
    
    add_executable(${tgt} ${tgt}.cpp)
    target_include_directories(${tgt} PRIVATE .)

    target_link_libraries(${tgt} PRIVATE VulkanEngine VulkanEngineUI)

    set_target_properties(${tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples/${tgt}
    )

    foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${cfg} OUTPUTCONFIG_UP)
        set_target_properties(${tgt} PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${CMAKE_BINARY_DIR}/${cfg}/bin/examples/${tgt})
    
        if(EXISTS "${SOURCE_RESOURCES}")
            file(COPY "${SOURCE_RESOURCES}/" DESTINATION ${CMAKE_BINARY_DIR}/${cfg}/bin/examples/${tgt}/examples/resources)
        endif()
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