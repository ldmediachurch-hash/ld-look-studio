cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
option(BUILD_TESTS "Build test suite" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/cube-parser.cpp
    src/cube-parser.hpp
    src/look-library.cpp
    src/look-library.hpp
    src/look-filter.cpp
    src/look-filter.hpp
    src/snapshot-manager.cpp
    src/snapshot-manager.hpp
    src/look-studio-dock.cpp
    src/look-studio-dock.hpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTS)
  enable_testing()
  add_executable(
    look-studio-tests
    tests/look-studio-tests.cpp
    src/cube-parser.cpp
    src/cube-parser.hpp
    src/look-library.cpp
    src/look-library.hpp
  )
  target_link_libraries(look-studio-tests PRIVATE Qt6::Core Qt6::Gui)
  target_include_directories(look-studio-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  set_target_properties(
    look-studio-tests
    PROPERTIES AUTOMOC ON CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
  )
  add_test(NAME look-studio-tests COMMAND look-studio-tests)
endif()
