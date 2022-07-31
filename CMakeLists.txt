cmake_minimum_required(VERSION 3.10)
 
project(sewer)

find_library(LIBUV_LIBRARY uv libuv)
find_path(LIBUV_INCLUDE_DIR NAMES uv.h)
 
include_directories(${LIBUV_INCLUDE_DIR})

set(COMMON_SRC
  src/common.h
  src/common.c
  src/sewer_socket.h    
  src/sewer_socket.c
)

add_executable(sewer_server src/sewer_server.c ${COMMON_SRC})
add_executable(sewer_cli src/sewer_cli.c ${COMMON_SRC})

target_link_libraries(sewer_server ${LIBUV_LIBRARY})
target_link_libraries(sewer_cli ${LIBUV_LIBRARY})
