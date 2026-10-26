cmake_minimum_required(VERSION 3.20)
project(path_tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

add_subdirectory(src/vulkan-base)

add_executable(path_tracer src/main.cpp)

target_link_libraries(path_tracer PRIVATE
    vulkan_base
    Vulkan::Vulkan
)

if(MSVC)
    # 源文件和执行字符集都用 UTF-8，消除 C4819
    add_compile_options(/utf-8)
    # 让 __cplusplus 宏正确报告 C++ 标准版本（glm 的 GLM_GTX_hash 提示靠它消除）
    add_compile_options(/Zc:__cplusplus)
endif()