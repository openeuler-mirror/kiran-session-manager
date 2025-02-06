cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE H_FILES ./*.h)
file(GLOB_RECURSE CPP_FILES ./*.cpp)
file(GLOB_RECURSE UI_FILES ./*.ui)

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  PROPERTIES CLASSNAME SessionManagerProxy NO_NAMESPACE true)

qt5_add_dbus_interface(
  SESSION_MANAGER_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  session_manager_interface)

set(TARGET_NAME kiran-session-window)

set(TS_FILES "${PROJECT_SOURCE_DIR}/translations/${TARGET_NAME}.zh_CN.ts")
execute_process(
  COMMAND
    lupdate-qt5 -no-obsolete ${PROJECT_SOURCE_DIR}/lib ${H_FILES} ${CPP_FILES} ${UI_FILES} -ts ${TS_FILES})

qt5_add_translation(QM_FILES ${TS_FILES})

add_executable(
  ${TARGET_NAME}
  ${H_FILES} ${CPP_FILES} ${UI_FILES} ${SESSION_MANAGER_SRCS} ${QM_FILES}
  "${PROJECT_SOURCE_DIR}/resources/resources.qrc")

target_include_directories(
  ${TARGET_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}
          ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include
          ${KIRAN_WIDGETS_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME} PRIVATE ${KIRAN_WIDGETS_LIBRARIES} Qt5::Widgets Qt5::DBus
                         KF5::ConfigCore lib-base)

install(FILES ${QM_FILES} DESTINATION ${KSM_INSTALL_TRANSLATIONDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
