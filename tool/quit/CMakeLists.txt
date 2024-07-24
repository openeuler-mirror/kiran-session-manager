cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE H_FILES ./*.h)
file(GLOB_RECURSE CPP_FILES ./*.cpp)

set(TARGET_NAME kiran-session-quit)

add_executable(
    ${TARGET_NAME}
    ${H_FILES}
    ${CPP_FILES})

target_include_directories(
    ${TARGET_NAME}
    PRIVATE ${PROJECT_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/include
            ${GTKMM_INCLUDE_DIRS}
            ${KLOG_GTK3_INCLUDE_DIRS})

target_link_libraries(${TARGET_NAME} PRIVATE 
  ${GTKMM_LIBRARIES}
  ${KLOG_GTK3_LIBRARIES})

install(TARGETS ${TARGET_NAME} DESTINATION ${KSM_INSTALL_BINDIR})
