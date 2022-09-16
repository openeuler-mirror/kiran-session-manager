cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SRC_H_FILES ./*.h)
file(GLOB_RECURSE SRC_CPP_FILES ./*.cpp)

qt_add_dbus_adaptor(
  SESSION_MANAGER_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  ${CMAKE_SOURCE_DIR}/src/core/session-manager.h Kiran::SessionManager
  session_manager_adaptor SessionManagerAdaptor)

qt_add_dbus_adaptor(
  CLIENT_PRIVATE_SRCS
  ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.ClientPrivate.xml
  ${CMAKE_SOURCE_DIR}/src/core/client/client-dbus.h
  Kiran::ClientDBus
  client_dbus_adaptor
  ClientDBusAdaptor)

qt_add_dbus_adaptor(
  PRESENCE_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.Presence.xml
  ${CMAKE_SOURCE_DIR}/src/core/presence.h Kiran::Presence presence_adaptor
  PresenceAdaptor)

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/data/com.kylinsec.Kiran.SessionManager.IdleMonitor.xml
  PROPERTIES CLASSNAME IdleMonitorProxy NO_NAMESPACE true)

qt_add_dbus_interface(
  IDLE_MONITOR_SRCS
  ${CMAKE_SOURCE_DIR}/data/com.kylinsec.Kiran.SessionManager.IdleMonitor.xml
  idle_monitor_proxy)

set(TARGET_NAME kiran-session-manager)

set(TS_FILES "${PROJECT_SOURCE_DIR}/translations/${TARGET_NAME}.zh_CN.ts")
qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(
  ${TARGET_NAME}
  ${SRC_H_FILES}
  ${SRC_CPP_FILES}
  ${SESSION_MANAGER_SRCS}
  ${CLIENT_PRIVATE_SRCS}
  ${PRESENCE_SRCS}
  ${IDLE_MONITOR_SRCS}
  ${QM_FILES})

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
          ${GSETTINGS_QT_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${GLIB2_LIBRARIES}
          ${X11_ICE_LIBRARIES}
          ${X11_SM_LIBRARIES}
          ${XEXT_LIBRARIES}
          ${XTRANS_LIBRARIES}
          ${GSETTINGS_QT_LIBRARIES}
          Qt5::Core
          KF5::ConfigCore
          KF5::KIOCore
          lib-base
          lib-dbus)

install(FILES ${QM_FILES} DESTINATION ${KSM_INSTALL_TRANSLATIONDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
