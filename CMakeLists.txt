cmake_minimum_required(VERSION 3.15)
project(cgame)

# Use static MSVC runtime (required for x64-windows-static triplet)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(cgame WIN32 src/main.cpp)

# Keep standard main() entry point instead of WinMain
if(MSVC)
    target_link_options(cgame PRIVATE /ENTRY:mainCRTStartup)
endif()

target_link_libraries(cgame PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
    imgui::imgui
    assimp::assimp
)
target_include_directories(cgame PRIVATE ${Stb_INCLUDE_DIR})

# Copy the executable to the project root after build (optional)
set_target_properties(cgame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
