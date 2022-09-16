cmake_minimum_required(VERSION 3.2)

project(kiran-session-manager VERSION 2.1)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(KSM REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_search_module(GIOMM REQUIRED giomm-2.4)
pkg_search_module(GTKMM REQUIRED gtkmm-3.0)
pkg_search_module(GIOUNIX REQUIRED gio-unix-2.0)
pkg_search_module(KLOG_GTK3 REQUIRED klog-gtk3)
pkg_search_module(JSONCPP REQUIRED jsoncpp)
pkg_search_module(GTEST REQUIRED gtest)
pkg_search_module(FMT REQUIRED fmt)
pkg_search_module(X11_ICE REQUIRED ice)
pkg_search_module(X11_SM REQUIRED sm)
pkg_search_module(XEXT REQUIRED xext)
pkg_search_module(XTRANS REQUIRED xtrans)

find_program(GDBUS_CODEGEN gdbus-codegen-glibmm3)

if(NOT GDBUS_CODEGEN)
  message(FATAL_ERROR "not found gdbus-codegen-glibmm3.")
endif()

message("gdbus-codegen: ${GDBUS_CODEGEN}")

configure_file(config.h.in ${PROJECT_BINARY_DIR}/config.h)

set_common_compiler_flags()

add_subdirectory(po)
add_subdirectory(data)
add_subdirectory(lib)
add_subdirectory(src)
# add_subdirectory(test)
