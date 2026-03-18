cmake_minimum_required(VERSION 3.16)
project(handheld_linux_client CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/../../src")
set(RAKNET_DIR "${PROJECT_SOURCE_DIR}/../lib_projects/raknet/jni/RaknetSources")

file(GLOB RAKNET_SOURCES "${RAKNET_DIR}/*.cpp")
add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${RAKNET_DIR}")
target_compile_options(raknet PRIVATE -Wno-narrowing)
target_compile_options(raknet PRIVATE -Wno-literal-suffix)

file(GLOB_RECURSE CLIENT_SOURCES "${SRC_DIR}/*.cpp")
file(GLOB SRC_RAKNET_SOURCES "${SRC_DIR}/raknet/*.cpp")

list(REMOVE_ITEM CLIENT_SOURCES
    "${SRC_DIR}/main_android.cpp"
    "${SRC_DIR}/main_android_java.cpp"
    "${SRC_DIR}/main_dedicated.cpp"
    "${SRC_DIR}/main_rpi.cpp"
    "${SRC_DIR}/AppPlatform_win32.cpp"
    "${SRC_DIR}/platform/audio/SoundSystemAL.cpp"
    "${SRC_DIR}/platform/audio/SoundSystemSL.cpp"
    "${SRC_DIR}/network/PHPDirectoryServer2.cpp"
    "${SRC_DIR}/network/NATPunchHandler.cpp"
)

list(REMOVE_ITEM CLIENT_SOURCES ${SRC_RAKNET_SOURCES})

add_executable(cobblestonium ${CLIENT_SOURCES})

target_compile_definitions(cobblestonium PRIVATE LINUX POSIX)

target_include_directories(cobblestonium PRIVATE
    "${SRC_DIR}"
    "${RAKNET_DIR}"
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(cobblestonium PRIVATE
    raknet
    OpenGL::GL
    PNG::PNG
    GLEW::GLEW
    Threads::Threads
    ${SDL2_LIBRARIES}
    dl
)
