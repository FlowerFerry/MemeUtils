
cmake_minimum_required(VERSION 3.20)

project(test-rec_writable_dir_path)

aux_source_directory(. SOURCES)

set(GIT_HOST "github.com" CACHE STRING "Git host")

include(FetchContent)

FetchContent_Declare(
	MemeString
	GIT_REPOSITORY https://${GIT_HOST}/FlowerFerry/MemeString.git
	GIT_TAG develop
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/MemeString
)
FetchContent_Populate(MemeString)

add_subdirectory(deps/MemeString)

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
	${CMAKE_CURRENT_SOURCE_DIR}/deps/MemeString/include
)

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${MEMESTRING_PROJECT_NAME}
)

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${MEMESTRING_PROJECT_NAME}>
	$<TARGET_FILE_DIR:${PROJECT_NAME}>
)