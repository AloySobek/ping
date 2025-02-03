cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

project(
  ft_ping
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC src/main.c src/config.c src/list.c src/socket.c src/print.c)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} m)
