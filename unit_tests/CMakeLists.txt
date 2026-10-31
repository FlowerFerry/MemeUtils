cmake_minimum_required(VERSION 3.20)

project(unit_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GIT_HOST "github.com" CACHE STRING "Git host")

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

include(FetchContent)

# ── Catch2 ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://${GIT_HOST}/catchorg/Catch2.git
  GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# ── MemeString ────────────────────────────────────────────────────────────────
FetchContent_Declare(
  MemeString
  GIT_REPOSITORY https://${GIT_HOST}/FlowerFerry/MemeString.git
  GIT_TAG develop
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/MemeString
)
FetchContent_Populate(MemeString)
add_subdirectory(deps/MemeString)

# ── concurrentqueue ───────────────────────────────────────────────────────────
FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://${GIT_HOST}/cameron314/concurrentqueue.git
  GIT_TAG master
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/concurrentqueue
)
FetchContent_Populate(concurrentqueue)
add_subdirectory(deps/concurrentqueue)

# ── span-lite ─────────────────────────────────────────────────────────────────
FetchContent_Declare(
  span-lite
  GIT_REPOSITORY https://${GIT_HOST}/martinmoene/span-lite.git
  GIT_TAG master
  GIT_SUBMODULES ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/span-lite
)
FetchContent_Populate(span-lite)

# ── yyjson ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://${GIT_HOST}/ibireme/yyjson.git
  GIT_TAG 0.10.0
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/yyjson
)
set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_FUZZER OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yyjson)

# ── rapidjson (header-only, populate only) ───────────────────────────────────
FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://${GIT_HOST}/Tencent/rapidjson.git
  GIT_TAG v1.1.0
  GIT_SUBMODULES ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/rapidjson
)
FetchContent_Populate(rapidjson)

# ── Test executable ───────────────────────────────────────────────────────────
set(SOURCES
    test_value_range.cpp
    test_log_level.cpp
    test_future_watcher.cpp
    test_dispatcher.cpp
    test_crc16.cpp
    test_locale_tag.cpp
    test_exprtk_extract.cpp
    test_exprtk_loop_guard.cpp
    test_exprtk_noncoded.cpp
    test_program_path.cpp
    test_span2d.cpp
    test_yyjson_number.cpp
    test_rapidjson_to_yyjson.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(PARENT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
cmake_path(NORMAL_PATH PARENT_INCLUDE_DIR)
message(STATUS "PARENT_INCLUDE_DIR: ${PARENT_INCLUDE_DIR}")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PARENT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/MemeString/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/span-lite/include
    ${rapidjson_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Catch2::Catch2WithMain
    ${MEMESTRING_PROJECT_NAME}
    concurrentqueue
    yyjson
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  $<TARGET_FILE:${MEMESTRING_PROJECT_NAME}>
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
