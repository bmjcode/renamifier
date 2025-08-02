cmake_minimum_required(VERSION 3.1.0)
include(FindPkgConfig)

project(renamifier VERSION 0.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6Test REQUIRED)
pkg_search_module(POPPLER REQUIRED poppler-qt6)
qt_standard_project_setup()

enable_testing(true)

qt_add_library(renamifier-ui
               mainwindow.cpp
               renderer.cpp
               viewer.cpp)
target_include_directories(renamifier-ui
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier-ui
                      PRIVATE Qt6::Widgets ${POPPLER_LIBRARIES})

qt_add_executable(renamifier WIN32
                  renamifier.cpp
                  renamifier.qrc)
target_include_directories(renamifier
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier
                      PRIVATE Qt6::Widgets ${POPPLER_LIBRARIES}
                      renamifier-ui)

qt_add_executable(renamifier-test
                  test.cpp
                  test.qrc)
target_include_directories(renamifier-test
                           SYSTEM PUBLIC ${POPPLER_INCLUDE_DIRS})
target_link_libraries(renamifier-test
                      PRIVATE Qt6::Widgets Qt6::Test ${POPPLER_LIBRARIES}
                      renamifier-ui)
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
