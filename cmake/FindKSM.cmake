include(GNUInstallDirs)

set(KSM_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(KSM_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
set(KSM_INSTALL_DATADIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
set(KSM_INSTALL_INCLUDE ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
set(GETTEXT_PACKAGE ${PROJECT_NAME})

macro(GEN_DBUS_CPP UPPER LOWER INTERFACE_PREFIX XML_PATH)

SET (${UPPER}_INTROSPECTION_XML ${XML_PATH})

SET (${UPPER}_GENERATED_CPP_FILES
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_stub.cpp
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_stub.h
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_common.cpp
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_common.h
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_proxy.cpp
    ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_proxy.h
)

ADD_CUSTOM_COMMAND (OUTPUT ${${UPPER}_GENERATED_CPP_FILES}
                    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/generated/
                    COMMAND ${GDBUS_CODEGEN} --generate-cpp-code=${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus
                            --interface-prefix=${INTERFACE_PREFIX}
                            ${${UPPER}_INTROSPECTION_XML}
                    DEPENDS ${${UPPER}_INTROSPECTION_XML}
                    COMMENT "Generate the stub for the ${LOWER}")
endmacro()