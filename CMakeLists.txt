cmake_minimum_required(VERSION 3.10)
 
project(sewer)

find_library(LIBUV_LIBRARY uv libuv)
find_path(LIBUV_INCLUDE_DIR NAMES uv.h)
 
include_directories(${LIBUV_INCLUDE_DIR})

set(COMMON_SRC
  src/common.h
  src/common.c
  src/sewer.h    
  src/sewer.c
)

add_executable(sewer src/sewer.c ${COMMON_SRC})

target_link_libraries(sewer ${LIBUV_LIBRARY})
