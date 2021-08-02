
cmake_minimum_required(VERSION 3.0)

find_program(MSGFMT NAMES "msgfmt" REQUIRED)

add_custom_target(ZH_MO ALL 
${MSGFMT} zh_CN.po -D ${PROJECT_SOURCE_DIR}/po/ -o ${PROJECT_BINARY_DIR}/zh_CN.mo)

install(FILES ${PROJECT_BINARY_DIR}/zh_CN.mo
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATA}/locale/zh_CN/LC_MESSAGES/"
        RENAME ${PROJECT_NAME}.mo)