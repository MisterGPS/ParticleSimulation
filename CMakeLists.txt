cmake_minimum_required(VERSION 3.24)
project(ParticleSimulation)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_compile_definitions(RELEASE)
    message("Added compile definition _RELEASE")
elseif (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_definitions(DEBUG)
    message("Added compile definition _DEBUG")
endif()

option(BENCHMARK "Build with fixed timestep and automatic exit for benchmarking" OFF)
if (BENCHMARK)
    add_compile_definitions(BENCHMARK)
    message("Added compile definition BENCHMARK")
endif()

set(LIBRARY_DIRECTORY "${CMAKE_SOURCE_DIR}/../Libraries")
set(INCLUDE_DIRECTORY "${LIBRARY_DIRECTORY}/include")
set(LIBS_DIRECTORY "${LIBRARY_DIRECTORY}/libs")

set(glad_INCLUDE_DIRECTORY ${INCLUDE_DIRECTORY}/glad)
set(GLFW_INCLUDE_DIRECTORY ${INCLUDE_DIRECTORY}/GLFW)
set(KHR_INCLUDE_DIRECTORY ${INCLUDE_DIRECTORY}/KHR)
set(GLM_INCLUDE_DIRECTORY ${INCLUDE_DIRECTORY}/glm)

set(GLFW_LIBRARY "${LIBS_DIRECTORY}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/glfw3.lib")
message("Using GLFW: ${GLFW_LIBRARY}")

add_executable(ParticleSimulation
        src/Private/main.cpp
        ${LIBRARY_DIRECTORY}/src/glad.c
        src/Public/Particle.h   src/Private/Particle.cpp
        src/Public/Shader.h     src/Private/Shader.cpp
        src/Public/Camera.h     src/Private/Camera.cpp
        src/Public/QuadTree.h   src/Private/QuadTree.cpp
        src/Shaders/ShadersInComp.h)

target_compile_options(ParticleSimulation PRIVATE /openmp)

add_custom_command(TARGET ParticleSimulation PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/shaders/ $<TARGET_FILE_DIR:ParticleSimulation>/Shaders)

set_target_properties(ParticleSimulation PROPERTIES
            LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

target_include_directories(ParticleSimulation PUBLIC ${glad_INCLUDE_DIRECTORY} ${GLFW_INCLUDE_DIRECTORY}
        ${KHR_INCLUDE_DIRECTORY} ${GLM_INCLUDE_DIRECTORY})
target_link_libraries(ParticleSimulation ${GLFW_LIBRARY})
