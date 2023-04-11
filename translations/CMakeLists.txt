
cmake_minimum_required(VERSION 3.0)

find_program(MSGFMT NAMES "msgfmt" REQUIRED)

file(GLOB TS_FILES *.po)
foreach(TS_FILE ${TS_FILES})
    string(REGEX REPLACE .*/ "" tmp ${TS_FILE})
    string(REGEX REPLACE \\..* "" TS_FILE_NAME ${tmp})
    add_custom_target(${TS_FILE_NAME}_MO ALL ${MSGFMT}
        ${TS_FILE} -D ${PROJECT_SOURCE_DIR}/po/ -o
        ${PROJECT_BINARY_DIR}/${TS_FILE_NAME}.mo)
    install(
        FILES ${PROJECT_BINARY_DIR}/${TS_FILE_NAME}.mo
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/locale/${TS_FILE_NAME}/LC_MESSAGES/"
        RENAME ${PROJECT_NAME}.mo)
endforeach()
