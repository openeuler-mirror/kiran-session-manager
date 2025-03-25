cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE H_FILES ./*.h)
file(GLOB_RECURSE CPP_FILES ./*.cpp)

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  PROPERTIES CLASSNAME SessionManagerProxy NO_NAMESPACE true)

qt5_add_dbus_interface(
  SESSION_MANAGER_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  session_manager_proxy)

set(TARGET_NAME kiran-session-quit)

set(TS_FILES "${PROJECT_SOURCE_DIR}/translations/${TARGET_NAME}.zh_CN.ts")
execute_process(COMMAND lupdate-qt5 -no-obsolete ${H_FILES} ${CPP_FILES} -ts
                        ${TS_FILES})

qt5_add_translation(QM_FILES ${TS_FILES})

add_executable(${TARGET_NAME} ${H_FILES} ${CPP_FILES} ${SESSION_MANAGER_SRCS}
                              ${QM_FILES})

target_include_directories(
  ${TARGET_NAME} PRIVATE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
                         ${PROJECT_SOURCE_DIR}/include ${KLOG_QT5_INCLUDE_DIRS})

target_link_libraries(${TARGET_NAME} PRIVATE ${KLOG_QT5_LIBRARIES} Qt5::Widgets
                                             Qt5::DBus)

install(FILES ${QM_FILES} DESTINATION ${KSM_INSTALL_TRANSLATIONDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
