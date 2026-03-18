cmake_minimum_required(VERSION 3.16)
project(handheld_linux_client CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
    find_package(PNG REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(Threads REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

set(SRC_DIR "${PROJECT_SOURCE_DIR}/../../src")
set(RAKNET_DIR "${PROJECT_SOURCE_DIR}/../lib_projects/raknet/jni/RaknetSources")

file(GLOB RAKNET_SOURCES "${RAKNET_DIR}/*.cpp")
add_library(raknet STATIC ${RAKNET_SOURCES})
target_include_directories(raknet PUBLIC "${RAKNET_DIR}")
target_compile_options(raknet PRIVATE -Wno-narrowing)
if(NOT EMSCRIPTEN)
    target_compile_options(raknet PRIVATE -Wno-literal-suffix)
endif()

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

if(EMSCRIPTEN)
    target_compile_definitions(raknet PRIVATE
        NO_NETWORK
        _RAKNET_SUPPORT_FileOperations=0
    )
    target_compile_options(raknet PRIVATE "-std=gnu++98")
endif()

list(REMOVE_ITEM CLIENT_SOURCES ${SRC_RAKNET_SOURCES})

add_executable(cobblestonium ${CLIENT_SOURCES})

target_compile_definitions(cobblestonium PRIVATE LINUX POSIX)
if(EMSCRIPTEN)
    target_compile_definitions(cobblestonium PRIVATE NO_NETWORK OPENGL_ES)
endif()

target_include_directories(cobblestonium PRIVATE
    "${SRC_DIR}"
    "${RAKNET_DIR}"
)

if(NOT EMSCRIPTEN)
    target_include_directories(cobblestonium PRIVATE
        ${SDL2_INCLUDE_DIRS}
    )
endif()

if(EMSCRIPTEN)
    set_target_properties(cobblestonium PROPERTIES SUFFIX ".html")

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
else()
    target_link_libraries(cobblestonium PRIVATE
        raknet
        OpenGL::GL
        PNG::PNG
        GLEW::GLEW
        Threads::Threads
        ${SDL2_LIBRARIES}
        dl
    )
endif()
