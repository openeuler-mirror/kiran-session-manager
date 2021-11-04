cmake_minimum_required(VERSION 3.2)

 # gschema.xml
 file(GLOB SCHEMA_IN_FILES ./*.gschema.xml.in)

 foreach(SCHEMA_IN_FILE IN LISTS SCHEMA_IN_FILES)
     string(REGEX REPLACE ".+/(.+)\\..*" "\\1" SCHEMA_FILE ${SCHEMA_IN_FILE})
     configure_file(${SCHEMA_IN_FILE} ${PROJECT_BINARY_DIR}/data/${SCHEMA_FILE})

     install(FILES ${PROJECT_BINARY_DIR}/data/${SCHEMA_FILE}
             DESTINATION  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas/)    
 endforeach()


 # desktop
configure_file(${PROJECT_SOURCE_DIR}/data//kiran.desktop.in ${PROJECT_BINARY_DIR}/data/kiran.desktop)
install(FILES ${PROJECT_BINARY_DIR}/data/kiran.desktop
              DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/xsessions)


# gresource
set (GRESOURCE_NAME ksm-gresource)              
find_program(GLIB_COMPILE_RESOURCE NAMES "glib-compile-resources")

add_custom_target(${GRESOURCE_NAME} ALL 
    ${GLIB_COMPILE_RESOURCE} ${PROJECT_SOURCE_DIR}/data/kiran-session-manager.gresource.xml --target kiran-session-manager.gresource --sourcedir ${PROJECT_SOURCE_DIR})

install(FILES ${PROJECT_BINARY_DIR}/data/kiran-session-manager.gresource
        DESTINATION "${KSM_INSTALL_DATADIR}/")

# txt
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/blacklist_autostart_apps.txt
         DESTINATION "${KSM_INSTALL_DATADIR}/")