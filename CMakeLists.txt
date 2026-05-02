cmake_minimum_required(VERSION 3.24)

project(KHORS_RayTracer VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.6
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

set(IMGUI_BUILD_FREETYPE ON CACHE BOOL "" FORCE)
set(IMGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(imgui)

FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(volk)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE ALL_HEADER_DIRS LIST_DIRECTORIES true src/*)
set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src)
foreach(DIR ${ALL_HEADER_DIRS})
    if(IS_DIRECTORY ${DIR})
        list(APPEND INCLUDE_DIRS ${DIR})
    endif()
endforeach()
add_executable(KHORS_RayTracer ${SOURCES}
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_custom_command(TARGET KHORS_RayTracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:KHORS_RayTracer>/assets
)

add_library(glad external/glad/src/gl.c)
add_library(stb_image STATIC external/stb/stb_img_impl.cpp)
add_library(vma STATIC external/vma/vk_mem_alloc.cpp)

target_include_directories(KHORS_RayTracer PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${INCLUDE_DIRS}
)
target_include_directories(glad PUBLIC external/glad/include)
target_include_directories(stb_image PUBLIC external/stb/include)
target_include_directories(vma PUBLIC external/vma/include)
target_link_libraries(vma PUBLIC volk::volk_headers Vulkan::Vulkan)
# find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

find_library(SHADERC_LIB NAMES shaderc_combined REQUIRED)
target_link_libraries(KHORS_RayTracer
    PRIVATE
        glfw
        OpenGL::GL
        Vulkan::Vulkan 
        volk::volk_headers
        ${SHADERC_LIB}
        glad
        stb_image
        vma
)
add_custom_target(upd_shaders
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            ${CMAKE_BINARY_DIR}/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/shaders
            ${CMAKE_BINARY_DIR}/assets/shaders
    COMMENT "shaders updated"
)
add_dependencies(KHORS_RayTracer upd_shaders)
