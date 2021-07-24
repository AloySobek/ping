cmake_minimum_required(VERSION 3.5)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(ping VERSION 0.1 LANGUAGES C)

set(SRC src/main.c)

include_directories(include)
add_executable(${PROJECT_NAME} ${SRC})