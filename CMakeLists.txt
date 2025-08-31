cmake_minimum_required(VERSION 3.20)
project(VulkanEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

file(GLOB_RECURSE source_files 
    src/*.cpp
)

add_executable(VulkanEngine
    ${source_files}
)

target_compile_definitions(VulkanEngine PRIVATE VOLK_STATIC_DEFINES)
target_link_libraries(VulkanEngine PRIVATE volk Vulkan::Headers glfw)

target_include_directories(VulkanEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VulkanEngine PRIVATE ENABLE_VULKAN_VALIDATION)
endif()