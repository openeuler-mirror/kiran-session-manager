cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE IDLE_H_FILES ./*.h)
file(GLOB_RECURSE IDLE_CPP_FILES ./*.cpp)

qt5_add_dbus_adaptor(
  IDLE_MONITOR_SRCS
  ${CMAKE_SOURCE_DIR}/data/com.kylinsec.Kiran.SessionManager.IdleMonitor.xml
  ${CMAKE_SOURCE_DIR}/src/idle/idle-monitor.h
  Kiran::IdleMonitor
  idle_monitor_adaptor
  IdleMonitorAdaptor)

set(TARGET_NAME kiran-session-idlemonitor)

add_executable(${TARGET_NAME} ${IDLE_H_FILES} ${IDLE_CPP_FILES}
                              ${IDLE_MONITOR_SRCS})

target_include_directories(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/include ${GSETTINGS_QT_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME} PRIVATE ${GSETTINGS_QT_LIBRARIES} Qt5::Widgets Qt5::DBus
                         KF5::IdleTime lib-base)

install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
