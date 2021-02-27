cmake_minimum_required(VERSION 3.1.0)
include(FindPkgConfig)

project(renamifier VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5Test REQUIRED)
pkg_search_module(POPPLER REQUIRED poppler-qt5)

enable_testing(true)

add_library(renamifier-ui
            mainwindow.cpp
            renderer.cpp
            viewer.cpp)
target_include_directories(renamifier-ui
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier-ui
                      Qt5::Widgets ${POPPLER_LIBRARIES})

add_executable(renamifier WIN32
               renamifier.cpp
               renamifier.qrc)
target_include_directories(renamifier
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier
                      Qt5::Widgets ${POPPLER_LIBRARIES} renamifier-ui)

add_executable(renamifier-test
               test.cpp
               test.qrc)
target_include_directories(renamifier-test
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier-test
                      Qt5::Widgets Qt5::Test ${POPPLER_LIBRARIES} renamifier-ui)
add_test(NAME renamifier-test COMMAND renamifier-test)

if(WIN32)
    # Deploy the Qt runtime
    add_custom_command(TARGET renamifier
                       POST_BUILD
                       COMMAND windeployqt
                           -xml
                           --compiler-runtime
                           --dir $<TARGET_FILE_DIR:renamifier>
                           $<TARGET_FILE:renamifier>)

    # Copy the NSIS script to build the installer
    file(COPY ${CMAKE_SOURCE_DIR}/winsetup.nsi
         DESTINATION ${CMAKE_BINARY_DIR})

    # Copy the license for the installer, and convert it to DOS format
    file(COPY ${CMAKE_SOURCE_DIR}/COPYING
         DESTINATION ${CMAKE_BINARY_DIR})
    add_custom_command(TARGET renamifier
                       POST_BUILD
                       COMMAND unix2dos
                           ${CMAKE_BINARY_DIR}/COPYING)
endif(WIN32)
