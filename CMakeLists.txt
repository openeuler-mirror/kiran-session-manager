cmake_minimum_required(VERSION 3.2)

project(kiran-session-manager VERSION 2.3)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(KSM REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets DBus LinguistTools)
find_package(KF5Config)
find_package(KF5KIO)
find_package(KF5IdleTime)

pkg_search_module(KLOG_QT5 REQUIRED klog-qt5)
pkg_search_module(GTEST REQUIRED gtest)
pkg_search_module(FMT REQUIRED fmt)
pkg_search_module(X11_ICE REQUIRED ice)
pkg_search_module(X11_SM REQUIRED sm)
pkg_search_module(XEXT REQUIRED xext)
pkg_search_module(XTRANS REQUIRED xtrans)
pkg_search_module(GSETTINGS_QT REQUIRED gsettings-qt)
pkg_search_module(GLIB2 REQUIRED glib-2.0)
pkg_search_module(KIRAN_STYLE REQUIRED kiran-style-helper)

configure_file(config.h.in ${PROJECT_BINARY_DIR}/config.h)

# add_subdirectory(po)
add_subdirectory(data)
add_subdirectory(lib)
add_subdirectory(src)
# add_subdirectory(test)
