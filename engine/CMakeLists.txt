cmake_minimum_required(VERSION 3.20)

project(IggyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(IGGY_ENGINE_SOURCES)

include(cmake/iggy_core_sources.cmake)
include(cmake/iggy_modules_sources.cmake)
include(cmake/iggy_runtime_sources.cmake)
include(cmake/iggy_scene_sources.cmake)
include(cmake/iggy_servers_sources.cmake)

add_library(iggy_engine ${IGGY_ENGINE_SOURCES})

target_include_directories(iggy_engine PUBLIC src)

enable_testing()

function(iggy_add_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE iggy_engine)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

include(cmake/iggy_core_tests.cmake)
include(cmake/iggy_modules_tests.cmake)
include(cmake/iggy_runtime_tests.cmake)
include(cmake/iggy_scene_tests.cmake)
include(cmake/iggy_servers_tests.cmake)

include(cmake/iggy_qt_shell.cmake)
