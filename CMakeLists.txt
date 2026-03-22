cmake_minimum_required(VERSION 3.20)
project(VulkanEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UP)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/bin)
endforeach()

#volk
set(VOLK_STATIC_DEFINES ON  CACHE BOOL "" FORCE)
set(VOLK_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/Vulkan-Headers)
add_subdirectory(third_party/volk)

target_include_directories(volk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include
)

#glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

add_subdirectory(third_party/glslang)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glm)

file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp
)
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/ui/.*")

add_library(VulkanEngine
    ${SOURCE_FILES}
)

target_compile_features(VulkanEngine PUBLIC cxx_std_17)
target_compile_definitions(VulkanEngine PUBLIC 
    VOLK_STATIC_DEFINES
    $<$<BOOL:${BUILD_SHARED_LIBS}>:VULKAN_ENGINE_SHARED>
)
target_link_libraries(VulkanEngine PUBLIC 
    Vulkan::Headers volk glfw glslang glslang-default-resource-limits)

set(ENGINE_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/collections
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/pipelines
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/feature_sets
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/feature_sets/graph
)

target_include_directories(VulkanEngine PUBLIC
    ${ENGINE_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang/glslang/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang/glslang/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VulkanEngine PUBLIC ENABLE_VULKAN_VALIDATION)
    target_compile_definitions(VulkanEngine PUBLIC ENGINE_LOG)
endif()

option(VULKAN_ENGINE_IMGUI_UI "Add Imgui UI" ON)
if (VULKAN_ENGINE_IMGUI_UI)

    file(GLOB IMGUI_SOURCE
        third_party/imgui/*.cpp
        third_party/imgui/*.h
    )

    file(GLOB UI_SOURCE
        "src/ui/*.cpp"
    )

    add_library(VulkanEngineUI STATIC 
        ${IMGUI_SOURCE}
        ${UI_SOURCE}
        third_party/imgui/backends/imgui_impl_glfw.h
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_vulkan.h
        third_party/imgui/backends/imgui_impl_vulkan.cpp
    )

    target_compile_definitions(VulkanEngineUI PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
    target_compile_definitions(VulkanEngineUI PUBLIC VOLK_STATIC_DEFINES)

    target_link_libraries(VulkanEngineUI PUBLIC
        VulkanEngine
    )

    target_include_directories(VulkanEngineUI PUBLIC
        ${ENGINE_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    )

    
endif()

option(VULKAN_ENGINE_BUILD_EXAMPLES "Build examples" ON)
if (VULKAN_ENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()