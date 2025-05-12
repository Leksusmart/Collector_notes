set(PROJECT_NAME "Collector_notes")
cmake_minimum_required(VERSION 3.16)

if (WIN32)
    project(${PROJECT_NAME} LANGUAGES CXX)
elseif(UNIX)
    project(${PROJECT_NAME})
endif()

set(CMAKE_CONFIGURATION_TYPES "Release;MinSizeRel" CACHE STRING "" FORCE)

#======================= INCLUSION OF Qt =======================#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_PREFIX_PATH $ENV{QTDIR})
find_package(Qt6Core REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Sql)

#=================== INCLUSION OF Project Files ====================#
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
include_directories(${SOURCE_DIR})

file(GLOB_RECURSE PROJECT_SOURCES
    "${SOURCE_DIR}/*.qrc"
    "${SOURCE_DIR}/*.rc"
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.h"
    "${SOURCE_DIR}/*.ui"
)
#=================== SETUP EXECTUABLE ====================#
# Add the forms directory to the AUTOUIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${SOURCE_DIR})
# Add the executable

if (WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})
elseif(UNIX)
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})

#===================== LINKING LIBRARIES =======================#
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_STATICPLUGIN)


target_link_libraries(${PROJECT_NAME} Qt6::Widgets Qt6::Sql)
