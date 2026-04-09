cmake_minimum_required(VERSION 3.10)
project(PhysicsStudio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(LINKING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Linking)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/LearnOpenGl/src)

# Includes
include_directories(${LINKING_DIR}/include)
include_directories(${SRC_DIR})
include_directories(${SRC_DIR}/Engine)
include_directories(${SRC_DIR}/Editor)

# Libraries
link_directories(${LINKING_DIR}/lib/GLFW)

# Source files
file(GLOB_RECURSE SOURCES 
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*.c"
    "${SRC_DIR}/*.h"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# OS Specific Links
if(WIN32)
    target_link_libraries(${PROJECT_NAME} glfw3 opengl32 user32 gdi32 shell32)
else()
    target_link_libraries(${PROJECT_NAME} glfw GL m dl pthread)
endif()

# Preprocessor definitions for ImGui
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
