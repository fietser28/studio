cmake_minimum_required(VERSION 3.12)
project(lvgl_runtime_v8.3)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 --no-entry")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s DEMANGLE_SUPPORT=1 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s NODEJS_CATCH_EXIT=0 -s NODEJS_CATCH_REJECTION=0 -s INITIAL_MEMORY=83886080 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=_malloc,_free -s EXPORTED_RUNTIME_METHODS=allocateUTF8,AsciiToString,UTF8ToString --pre-js ${PROJECT_SOURCE_DIR}/../common/pre.js --post-js ${PROJECT_SOURCE_DIR}/../common/post.js")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../common)

# lvgl
add_subdirectory(lvgl)

# EEZ Framework
add_definitions(-DEEZ_FOR_LVGL)
add_subdirectory(../../eez-framework [EXCLUDE_FROM_ALL])

# lvgl_runtime
file(GLOB_RECURSE SOURCES
    ../common/*.c
    ../common/src/*.cpp
    ../common/src/*.c
)

add_executable(lvgl_runtime_v8.3 ${SOURCES})

set(CMAKE_EXECUTABLE_SUFFIX ".html")

target_link_libraries(lvgl_runtime_v8.3
    lvgl
    eez-framework
)

#
add_custom_command(TARGET lvgl_runtime_v8.3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v8.3.js"
    "${PROJECT_SOURCE_DIR}/../../..")

add_custom_command(TARGET lvgl_runtime_v8.3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v8.3.js"
    "${PROJECT_SOURCE_DIR}/../../../../../../../build/project-editor/flow/runtime")

add_custom_command(TARGET lvgl_runtime_v8.3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v8.3.wasm"
    "${PROJECT_SOURCE_DIR}/../../..")

add_custom_command(TARGET lvgl_runtime_v8.3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_SOURCE_DIR}/build/lvgl_runtime_v8.3.wasm"
    "${PROJECT_SOURCE_DIR}/../../../../../../../build/project-editor/flow/runtime")