cmake_minimum_required(VERSION 3.20)

project(IggyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(iggy_engine
  src/core/math/Vec2.cpp
  src/core/resource/ResourceId.cpp
)

target_include_directories(iggy_engine PUBLIC src)

enable_testing()

add_executable(core_math_tests tests/core_math_tests.cpp)
target_link_libraries(core_math_tests PRIVATE iggy_engine)

add_executable(core_resource_tests tests/core_resource_tests.cpp)
target_link_libraries(core_resource_tests PRIVATE iggy_engine)

add_test(NAME core_math_tests COMMAND core_math_tests)
add_test(NAME core_resource_tests COMMAND core_resource_tests)
