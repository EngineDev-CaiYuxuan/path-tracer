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

# ---- MSVC 编译选项（放在 target 之前）----
if(MSVC)
    add_compile_options(/utf-8 /Zc:__cplusplus /wd4828)
endif()

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

add_executable(path_tracer
    src/main.cpp
    src/render/render.h
    src/render/render.cpp
    src/application.h
    src/application.cpp
)

target_link_libraries(path_tracer PRIVATE
    vulkan_base
    Vulkan::Vulkan
)

# ---- 定位 glslc ----
find_program(GLSL_COMPILER
    NAMES glslc glslc.exe
    HINTS $ENV{VULKAN_SDK}/Bin
)
if(NOT GLSL_COMPILER AND Vulkan_GLSLC_EXECUTABLE)
    set(GLSL_COMPILER ${Vulkan_GLSLC_EXECUTABLE})
endif()
if(NOT GLSL_COMPILER)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK or set VULKAN_SDK env var.")
endif()

# ---- shader 源路径 + 输出路径 ----
# ---- 编译 Shader ----
set(SHADER_SRC ${CMAKE_SOURCE_DIR}/assets/shaders/shader.comp)
set(SHADER_OUT $<TARGET_FILE_DIR:path_tracer>/assets/shaders/comp.spv) # 输出到 exe 同级的 assets/shaders/

# 首次配置时先检查源文件存在，早报错早发现
if(NOT EXISTS ${SHADER_SRC})
    message(FATAL_ERROR "shader source not found: ${SHADER_SRC}")
endif()

# ---- 独立编译 Shader ----
add_custom_target(compile_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:path_tracer>/assets/shaders
    COMMAND ${GLSL_COMPILER}
            ${SHADER_SRC}
            -o ${SHADER_OUT}
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling ${SHADER_SRC} -> comp.spv"
    VERBATIM
)
add_dependencies(path_tracer compile_shaders)

# ---- 拷贝模型 ----
add_custom_target(copy_models ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/models
            $<TARGET_FILE_DIR:path_tracer>/assets/models
    COMMENT "Copying models to output directory"
)
add_dependencies(path_tracer copy_models)
