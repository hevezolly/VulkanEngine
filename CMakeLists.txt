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

#glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

#volk
set(VOLK_STATIC_DEFINES ON  CACHE BOOL "" FORCE)
set(VOLK_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/volk)
add_subdirectory(third_party/Vulkan-Headers)

add_subdirectory(third_party/glslang)

file(GLOB_RECURSE source_files 
    src/*.cpp
)

add_library(VulkanEngine
    ${source_files}
)

target_compile_features(VulkanEngine PUBLIC cxx_std_17)
target_compile_definitions(VulkanEngine PUBLIC 
    VOLK_STATIC_DEFINES
    $<$<BOOL:${BUILD_SHARED_LIBS}>:VULKAN_ENGINE_SHARED>
)
target_link_libraries(VulkanEngine PUBLIC volk Vulkan::Headers glfw glslang glslang-default-resource-limits)

target_include_directories(VulkanEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/pipelines
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang/glslang/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glslang/glslang/Public
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VulkanEngine PUBLIC ENABLE_VULKAN_VALIDATION)
endif()


option(VULKAN_ENGINE_BUILD_EXAMPLES "Build examples" ON)
if (VULKAN_ENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()