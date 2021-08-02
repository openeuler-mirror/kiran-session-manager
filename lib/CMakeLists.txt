cmake_minimum_required(VERSION 3.0)

set (TARGET_NAME lib-base)

file(GLOB_RECURSE BASE_H_FILES ./*.h)
file(GLOB_RECURSE BASE_CPP_FILES ./*.cpp)

set (ALL_FILES ${BASE_H_FILES} ${BASE_CPP_FILES})

add_library(${TARGET_NAME} ${ALL_FILES})


target_include_directories(${TARGET_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${GIOMM_INCLUDE_DIRS}
    ${KLOG_GTK3_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS})


target_link_libraries(${TARGET_NAME} PUBLIC
    ${GIOMM_LIBRARIES}
    ${KLOG_GTK3_LIBRARIES}
    ${FMT_LIBRARIES})

