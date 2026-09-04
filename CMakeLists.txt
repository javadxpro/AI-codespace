cmake_minimum_required(VERSION 3.16)
project(KIMIA VERSION 0.1.0 LANGUAGES CXX)

option(KIMIA_WERROR "Treat warnings as errors" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KIMIA_STRICT_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion)

add_library(kimia_core
    Engine/Core/src/Log.cpp
    Engine/Core/src/Profiler.cpp
    Engine/Core/src/Time.cpp
)
target_include_directories(kimia_core PUBLIC Engine/Core/include)
target_compile_options(kimia_core PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_core PRIVATE -Werror)
endif()

add_library(kimia_math INTERFACE)
target_include_directories(kimia_math INTERFACE Engine/Math/include Engine/Core/include)

add_executable(kimia_tests
    Tests/src/main.cpp
    Tests/src/CoreTests.cpp
    Tests/src/MathTests.cpp
)
target_include_directories(kimia_tests PRIVATE Tests/include Engine/Core/include)
target_link_libraries(kimia_tests PRIVATE kimia_core kimia_math)
target_compile_options(kimia_tests PRIVATE ${KIMIA_STRICT_WARNINGS})
if(KIMIA_WERROR)
  target_compile_options(kimia_tests PRIVATE -Werror)
endif()

include(CTest)
add_test(NAME kimia_tests COMMAND kimia_tests)
