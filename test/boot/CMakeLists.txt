set(TARGET_NAME ksm-test-boot)

file(GLOB_RECURSE TEST_H_FILES ./*.h ${PROJECT_SOURCE_DIR}/src/core/*.h)
file(GLOB_RECURSE TEST_CPP_FILES ./*.cpp ${PROJECT_SOURCE_DIR}/src/core/*.cpp)
# 过滤main.cpp
list(REMOVE_ITEM TEST_CPP_FILES ${PROJECT_SOURCE_DIR}/src/core/main.cpp)


qt5_add_dbus_adaptor(
  SESSION_MANAGER_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  ${CMAKE_SOURCE_DIR}/src/core/session-manager.h Kiran::SessionManager
  session_manager_adaptor SessionManagerAdaptor)

qt5_add_dbus_adaptor(
  CLIENT_PRIVATE_SRCS
  ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.ClientPrivate.xml
  ${CMAKE_SOURCE_DIR}/src/core/client/client-dbus.h
  Kiran::ClientDBus
  client_dbus_adaptor
  ClientDBusAdaptor)

qt5_add_dbus_adaptor(
  PRESENCE_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.Presence.xml
  ${CMAKE_SOURCE_DIR}/src/core/presence.h Kiran::Presence presence_adaptor
  PresenceAdaptor)

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/data/com.kylinsec.Kiran.SessionManager.IdleMonitor.xml
  PROPERTIES CLASSNAME IdleMonitorProxy NO_NAMESPACE true)

qt5_add_dbus_interface(
  IDLE_MONITOR_SRCS
  ${CMAKE_SOURCE_DIR}/data/com.kylinsec.Kiran.SessionManager.IdleMonitor.xml
  idle_monitor_proxy)

add_executable(
  ${TARGET_NAME}
  ${TEST_H_FILES}
  ${TEST_CPP_FILES}
  ${SESSION_MANAGER_SRCS}
  ${CLIENT_PRIVATE_SRCS}
  ${PRESENCE_SRCS}
  ${IDLE_MONITOR_SRCS})

target_include_directories(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_BINARY_DIR}
          ${PROJECT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/include
          ${GLIB2_INCLUDE_DIRS}
          ${X11_ICE_INCLUDE_DIRS}
          ${X11_SM_INCLUDE_DIRS}
          ${XEXT_INCLUDE_DIRS}
          ${XTRANS_INCLUDE_DIRS}
          ${XCB_INCLUDE_DIRS}
          ${GSETTINGS_QT_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${GLIB2_LIBRARIES}
          ${X11_ICE_LIBRARIES}
          ${X11_SM_LIBRARIES}
          ${XEXT_LIBRARIES}
          ${XTRANS_LIBRARIES}
          ${XCB_LIBRARIES}
          ${GSETTINGS_QT_LIBRARIES}
          Qt5::Core
          Qt5::DBus
          Qt5::Test
          KF5::ConfigCore
          KF5::KIOCore
          lib-base
          lib-dbus)

add_test(NAME test-boot COMMAND ${TARGET_NAME})