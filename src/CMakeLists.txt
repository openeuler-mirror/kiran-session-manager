cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SRC_H_FILES ./*.h)
file(GLOB_RECURSE SRC_CPP_FILES ./*.cpp)

gen_dbus_cpp(SESSION_MANAGER session_manager org.
             ${CMAKE_SOURCE_DIR}/src/org.gnome.SessionManager.xml)
gen_dbus_cpp(CLIENT_PRIVATE client_private org.
             ${CMAKE_SOURCE_DIR}/src/org.gnome.SessionManager.ClientPrivate.xml)
gen_dbus_cpp(PRESENCE presence org.
             ${CMAKE_SOURCE_DIR}/src/org.gnome.SessionManager.Presence.xml)

set(TARGET_NAME kiran-session-manager)

add_executable(
  ${TARGET_NAME}
  ${SRC_H_FILES} ${SRC_CPP_FILES} ${SESSION_MANAGER_GENERATED_CPP_FILES}
  ${CLIENT_PRIVATE_GENERATED_CPP_FILES} ${PRESENCE_GENERATED_CPP_FILES})

target_include_directories(
  ${TARGET_NAME}
  PRIVATE ${PROJECT_BINARY_DIR}
          ${PROJECT_BINARY_DIR}/generated
          ${PROJECT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/include
          ${GTKMM_INCLUDE_DIRS}
          ${JSONCPP_INCLUDE_DIRS}
          ${X11_ICE_INCLUDE_DIRS}
          ${X11_SM_INCLUDE_DIRS}
          ${XEXT_INCLUDE_DIRS}
          ${XTRANS_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE ${GTKMM_LIBRARIES}
          ${JSONCPP_LIBRARIES}
          ${X11_ICE_LIBRARIES}
          ${X11_SM_LIBRARIES}
          ${XEXT_LIBRARIES}
          ${XTRANS_LIBRARIES}
          lib-base)

install(TARGETS ${TARGET_NAME} DESTINATION ${KSM_INSTALL_BINDIR})
