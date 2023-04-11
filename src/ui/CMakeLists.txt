cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE UI_H_FILES ./*.h)
file(GLOB_RECURSE UI_CPP_FILES ./*.cpp)
# file(GLOB_RECURSE UI_FILES2 ./*.ui)

set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  PROPERTIES CLASSNAME SessionManagerProxy NO_NAMESPACE true)

qt5_add_dbus_interface(
  SESSION_MANAGER_SRCS ${CMAKE_SOURCE_DIR}/data/org.gnome.SessionManager.xml
  session_manager_interface)

set(TARGET_NAME kiran-session-window)

qt5_wrap_ui(UI_FILES ${CMAKE_CURRENT_SOURCE_DIR}/exit-query-window.ui
            ${CMAKE_CURRENT_SOURCE_DIR}/inhibitor-row.ui)

file(GLOB TS_FILES "${PROJECT_SOURCE_DIR}/translations/${TARGET_NAME}.*.ts")
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

add_executable(
  ${TARGET_NAME}
  ${UI_H_FILES} ${UI_CPP_FILES} ${UI_FILES} ${SESSION_MANAGER_SRCS} ${QM_FILES}
  "${PROJECT_SOURCE_DIR}/resources/resources.qrc")

target_include_directories(
  ${TARGET_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}
          ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include
          ${KIRAN_STYLE_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME} PRIVATE ${KIRAN_STYLE_LIBRARIES} Qt5::Widgets Qt5::DBus
                         KF5::ConfigCore lib-base)

install(FILES ${QM_FILES} DESTINATION ${KSM_INSTALL_TRANSLATIONDIR})
install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
