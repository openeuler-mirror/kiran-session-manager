macro(list_join)
  foreach(elem IN LISTS ${ARGV0})
    string(APPEND ${ARGV2} ${ARGV1} ${elem})
  endforeach()
endmacro()

macro(SET_COMMON_COMPILER_FLAGS)
  list_join(KSM_COMPILER_FLAGS " " CMAKE_CXX_FLAGS)
  # list_join(KSM_COMPILER_FLAGS_DEBUG " " CMAKE_CXX_FLAGS_DEBUG)
  # list_join(KSM_COMPILER_FLAGS_RELEASE " " CMAKE_CXX_FLAGS_RELEASE)
endmacro()

macro(GEN_DBUS_CPP UPPER LOWER INTERFACE_PREFIX XML_PATH)

  set(${UPPER}_INTROSPECTION_XML ${XML_PATH})

  set(${UPPER}_GENERATED_CPP_FILES
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_stub.cpp
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_stub.h
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_common.cpp
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_common.h
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_proxy.cpp
      ${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus_proxy.h)

  add_custom_command(
    OUTPUT ${${UPPER}_GENERATED_CPP_FILES}
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/generated/
    COMMAND
      ${GDBUS_CODEGEN}
      --generate-cpp-code=${CMAKE_BINARY_DIR}/generated/${LOWER}_dbus
      --interface-prefix=${INTERFACE_PREFIX} ${${UPPER}_INTROSPECTION_XML}
    DEPENDS ${${UPPER}_INTROSPECTION_XML}
    COMMENT "Generate the stub for the ${LOWER}")
endmacro()
