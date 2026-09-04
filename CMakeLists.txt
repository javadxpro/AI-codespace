cmake_minimum_required(VERSION 3.16)
project(KIMIA VERSION 0.1.0 LANGUAGES CXX)

option(KIMIA_WERROR "Treat warnings as errors" ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(kimia_core
    Engine/Core/src/Log.cpp
    Engine/Core/src/Profiler.cpp
    Engine/Core/src/Time.cpp
)
target_include_directories(kimia_core PUBLIC Engine/Core/include)
target_compile_options(kimia_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion)
if(KIMIA_WERROR)
  target_compile_options(kimia_core PRIVATE -Werror)
endif()

add_executable(kimia_tests
    Tests/src/main.cpp
    Tests/src/CoreTests.cpp
)
target_include_directories(kimia_tests PRIVATE Tests/include Engine/Core/include)
target_link_libraries(kimia_tests PRIVATE kimia_core)
target_compile_options(kimia_tests PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion)
if(KIMIA_WERROR)
  target_compile_options(kimia_tests PRIVATE -Werror)
endif()

include(CTest)
add_test(NAME kimia_tests COMMAND kimia_tests)
