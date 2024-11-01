cmake_minimum_required(VERSION 3.5)

# Build type
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Project declaration
project(megatest)

add_subdirectory(libs)

# binary
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
				${CMAKE_SOURCE_DIR}/libs/glfw/include/
				${CMAKE_SOURCE_DIR}/libs/glad/include/
				${CMAKE_SOURCE_DIR}/libs/glm/
				${CMAKE_SOURCE_DIR}/libs/imgui/
				${CMAKE_SOURCE_DIR}/libs/imgui/backends/
				${CMAKE_SOURCE_DIR}/libs/stb/
				${CMAKE_SOURCE_DIR}/libs/lua/include/
				${CMAKE_SOURCE_DIR}/libs/yaml-cpp/include/
				${CMAKE_SOURCE_DIR}/libs/IconFontCppHeaders/
				${CMAKE_SOURCE_DIR}/libs/
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# static build gcc and everything
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad spdlog glm stb imgui lua54 sol2)
