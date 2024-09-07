cmake_minimum_required(VERSION 3.8)
project(file_utils)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wunused-variable")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 20)

# link libs only when needed
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")

# Compile as C++20
add_compile_options(-std=c++20)

# Dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)

# Boost
find_package(Boost REQUIRED)
list(APPEND INCLUDE_LIST ${Boost_INCLUDE_DIRS})
list(APPEND LIB_LIST ${Boost_LIBRARIES})

# Gflags
find_package(Gflags REQUIRED)
list(APPEND INCLUDE_LIST ${GFLAGS_INCLUDE_DIRS})
list(APPEND LIB_LIST ${GFLAGS_LIBRARIES})

# Glog
find_package(Glog REQUIRED)
list(APPEND INCLUDE_LIST ${GLOG_INCLUDE_DIRS})
list(APPEND LIB_LIST ${GLOG_LIBRARIES})

# GTest
find_package(GTest REQUIRED)
list(APPEND INCLUDE_LIST ${GTEST_INCLUDE_DIRS})
list(APPEND LIB_LIST GTest::gtest)

# csv-parser
list(APPEND INCLUDE_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/third-party/csv-parser/single_include)

# yaml-cpp
add_subdirectory(third-party/yaml-cpp)
list(APPEND INCLUDE_LIST ${YAML_CPP_INCLUDE_DIR})
list(APPEND LIB_LIST yaml-cpp::yaml-cpp)

include_directories(include ${INCLUDE_LIST})

add_library(${PROJECT_NAME} SHARED src/Files.cpp)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
ament_target_dependencies(${PROJECT_NAME} rclcpp rclpy)
target_link_libraries(${PROJECT_NAME} ${LIB_LIST})

# Visibility macros
target_compile_definitions(${PROJECT_NAME}
                           PUBLIC "${PROJECT_NAME}_BUILDING_LIBRARY")

# Installation and exports
install(DIRECTORY include/ DESTINATION include)

install(
  TARGETS ${PROJECT_NAME} yaml-cpp
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME})

ament_python_install_package(${PROJECT_NAME})

ament_package()
