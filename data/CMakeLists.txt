cmake_minimum_required(VERSION 3.2)

# xxx.in -> xxx
file(GLOB XXX_IN_FILES ./*.in)

foreach(XXX_IN_FILE IN LISTS XXX_IN_FILES)
  string(REGEX REPLACE ".+/(.+)\\..*" "\\1" XXX_FILE ${XXX_IN_FILE})
  configure_file(${XXX_IN_FILE} ${PROJECT_BINARY_DIR}/data/${XXX_FILE} @ONLY)
endforeach()

# wayland executable file
install(PROGRAMS ${PROJECT_BINARY_DIR}/data/kiran-session-wayland
                 ${PROJECT_BINARY_DIR}/data/kiran-session-kwayland
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}/)

# gschema.xml
install(
  FILES
    ${PROJECT_BINARY_DIR}/data/com.kylinsec.kiran.session-manager.gschema.xml
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas/)

# conf
install(FILES ${PROJECT_SOURCE_DIR}/data/90-lightdm-kiran-greeter.conf
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/lightdm/lightdm.conf.d)

# desktop
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

# xsession-errors logrotate conf
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/xsession-errors.logrotate
  DESTINATION "${SYSCONFDIR}/logrotate.d/"
  RENAME "xsession-errors"
)