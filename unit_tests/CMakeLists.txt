cmake_minimum_required(VERSION 3.20)

project(unit_tests)

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

# ── Test executable ───────────────────────────────────────────────────────────
set(SOURCES
    test_value_range.cpp
    test_log_level.cpp
    test_future_watcher.cpp
    test_dispatcher.cpp
    test_crc16.cpp
    test_locale_tag.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(PARENT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
cmake_path(NORMAL_PATH PARENT_INCLUDE_DIR)
message(STATUS "PARENT_INCLUDE_DIR: ${PARENT_INCLUDE_DIR}")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PARENT_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/MemeString/include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Catch2::Catch2WithMain
    ${MEMESTRING_PROJECT_NAME}
    concurrentqueue
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  $<TARGET_FILE:${MEMESTRING_PROJECT_NAME}>
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
