cmake_minimum_required(VERSION 3.16)
project(handheld_web_client CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/../../src")
set(RAKNET_DIR "${PROJECT_SOURCE_DIR}/../lib_projects/raknet/jni/RaknetSources")

file(GLOB RAKNET_SOURCES "${RAKNET_DIR}/*.cpp")
add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${RAKNET_DIR}")
target_compile_options(raknet PRIVATE -Wno-narrowing "-std=gnu++98")
target_compile_definitions(raknet PRIVATE
    NO_NETWORK
    _RAKNET_SUPPORT_FileOperations=0
)

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
set_target_properties(cobblestonium PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")

target_compile_definitions(cobblestonium PRIVATE POSIX NO_NETWORK OPENGL_ES WEB)
target_include_directories(cobblestonium PRIVATE
    "${SRC_DIR}"
    "${RAKNET_DIR}"
)

target_compile_options(cobblestonium PRIVATE
    "-sUSE_SDL=2"
    "-sUSE_LIBPNG=1"
    "-Wno-c++11-narrowing"
)

target_link_options(cobblestonium PRIVATE
    "-lidbfs.js"
    "-sUSE_SDL=2"
    "-sUSE_LIBPNG=1"
    "-sFORCE_FILESYSTEM=1"
    "-sNO_EXIT_RUNTIME=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sLEGACY_GL_EMULATION=1"
    "-sGL_UNSAFE_OPTS=0"
    "-sMAX_WEBGL_VERSION=2"
    "--shell-file" "${PROJECT_SOURCE_DIR}/web_shell.html"
    "--preload-file" "${PROJECT_SOURCE_DIR}/../../data@/data"
)

target_link_libraries(cobblestonium PRIVATE
    raknet
)

add_custom_command(TARGET cobblestonium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/manifest.webmanifest"
        "$<TARGET_FILE_DIR:cobblestonium>/manifest.webmanifest"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/service-worker.js"
        "$<TARGET_FILE_DIR:cobblestonium>/service-worker.js"
)
