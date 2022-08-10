cmake_minimum_required(VERSION 3.2)

# gschema.xml
file(GLOB SCHEMA_IN_FILES ./*.gschema.xml.in)

foreach(SCHEMA_IN_FILE IN LISTS SCHEMA_IN_FILES)
  string(REGEX REPLACE ".+/(.+)\\..*" "\\1" SCHEMA_FILE ${SCHEMA_IN_FILE})
  configure_file(${SCHEMA_IN_FILE} ${PROJECT_BINARY_DIR}/data/${SCHEMA_FILE})

  install(FILES ${PROJECT_BINARY_DIR}/data/${SCHEMA_FILE}
          DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas/)
endforeach()

# conf
install(FILES ${PROJECT_SOURCE_DIR}/data/90-lightdm-kiran-greeter.conf
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/lightdm/lightdm.conf.d)

# desktop.in
file(GLOB DESKTOP_IN_FILES ./*.desktop.in)
foreach(DESKTOP_IN_FILE IN LISTS DESKTOP_IN_FILES)
  string(REGEX REPLACE ".+/(.+)\\..*" "\\1" DESKTOP_FILE ${DESKTOP_IN_FILE})
  configure_file(${DESKTOP_IN_FILE} ${PROJECT_BINARY_DIR}/data/${DESKTOP_FILE})
endforeach()

install(FILES ${PROJECT_BINARY_DIR}/data/kiran.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/xsessions)
install(FILES ${PROJECT_BINARY_DIR}/data/kiran-wayland.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/wayland-sessions)
install(FILES ${PROJECT_BINARY_DIR}/data/kiran-greeter.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/xgreeters/)
install(FILES ${PROJECT_BINARY_DIR}/data/kiran-session-idlemonitor.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications/)

# txt
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/blacklist_autostart_apps.txt
        DESTINATION "${KSM_INSTALL_DATADIR}/")

# sessions
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/sessions/kiran.session
        ${CMAKE_CURRENT_SOURCE_DIR}/sessions/kiran-greeter.session
        ${CMAKE_CURRENT_SOURCE_DIR}/sessions/kiran-wayland.session
  DESTINATION "${KSM_INSTALL_DATADIR}/sessions/")
