# 收集基础层所有 .cpp
file(GLOB_RECURSE VULKAN_BASE_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(vulkan_base STATIC ${VULKAN_BASE_SOURCES})

# 头文件路径：当前目录 + stb + tiny
target_include_directories(vulkan_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/stb
    ${PROJECT_SOURCE_DIR}/third_party/tiny
)

# 链接依赖：Vulkan + GLFW + glm
target_link_libraries(vulkan_base PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
)