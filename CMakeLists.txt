cmake_minimum_required(VERSION 3.16)
project(BusBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(BusBook
  src/concepts.cpp
  src/main.cpp
  src/models.cpp
  src/storage.cpp
  src/system.cpp
)

target_include_directories(BusBook PRIVATE include)

if (MSVC)
  target_compile_options(BusBook PRIVATE /W4 /permissive-)
else()
  target_compile_options(BusBook PRIVATE -Wall -Wextra -pedantic)
endif()

enable_testing()

add_executable(BusBookTests
  tests/busbook_tests.cpp
  src/concepts.cpp
  src/models.cpp
  src/storage.cpp
)

target_include_directories(BusBookTests PRIVATE include)

if (MSVC)
  target_compile_options(BusBookTests PRIVATE /W4 /permissive-)
else()
  target_compile_options(BusBookTests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME BusBookUnitTests COMMAND BusBookTests)

add_executable(BusBookSystemFlowTests
  tests/system_flow_tests.cpp
  src/concepts.cpp
  src/models.cpp
  src/storage.cpp
  src/system.cpp
)

target_include_directories(BusBookSystemFlowTests PRIVATE include)

if (MSVC)
  target_compile_options(BusBookSystemFlowTests PRIVATE /W4 /permissive-)
else()
  target_compile_options(BusBookSystemFlowTests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME BusBookSystemFlowTests COMMAND BusBookSystemFlowTests)
