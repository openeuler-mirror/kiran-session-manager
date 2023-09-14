cmake_minimum_required(VERSION 3.0)

set(TARGET_NAME lib-base)

file(GLOB_RECURSE BASE_H_FILES ./*.h)
file(GLOB_RECURSE BASE_CPP_FILES ./*.cpp)

set(ALL_FILES ${BASE_H_FILES} ${BASE_CPP_FILES})

add_library(${TARGET_NAME} STATIC ${ALL_FILES})

target_include_directories(
  ${TARGET_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include
         ${PROJECT_BINARY_DIR} ${KLOG_QT5_INCLUDE_DIRS} ${FMT_INCLUDE_DIRS}
         ${JSONCPP_INCLUDE_DIRS})

target_link_libraries(
  ${TARGET_NAME} PUBLIC Qt5::Core ${KLOG_QT5_LIBRARIES} ${FMT_LIBRARIES}
                        ${JSONCPP_LIBRARIES})
