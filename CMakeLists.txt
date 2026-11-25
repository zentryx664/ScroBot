cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
elseif (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
project(ScroBot VERSION 2.0.0)

# Every .cpp under src/ is built automatically: add a feature = add a file.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Set GEODE_SDK env var.")
endif()
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

# Screen Record uses EGL and OpenGL ES directly on Android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} GLESv2 EGL)
endif()
