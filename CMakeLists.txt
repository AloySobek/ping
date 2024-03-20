cmake_minimum_required(VERSION 3.5)

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

project(
  ft_ping
  VERSION 0.1
  LANGUAGES C)

set(SRC src/main.c src/cli.c)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SRC})
