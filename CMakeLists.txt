cmake_minimum_required(VERSION 3.10)

# Read version from the package.xml file.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/package.xml package_xml_str)

if(NOT package_xml_str MATCHES "<version>([0-9]+.[0-9]+.[0-9]+)</version>")
  message(FATAL_ERROR "Could not parse project version from package.xml. Aborting.")
endif()

project(symmetri VERSION ${CMAKE_MATCH_1})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic")

if(BUILD_TESTING)
  enable_testing()
  if(ASAN_BUILD AND NOT TSAN_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined -fno-omit-frame-pointer -O0")
  elseif(TSAN_BUILD AND NOT ASAN_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer -O0")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-tsan")
    endif()

  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=auto -O3")
  endif()
endif()

include(GNUInstallDirs)

# the lib
add_subdirectory(symmetri)

# some examples using the lib
if(BUILD_EXAMPLES)
  add_subdirectory(examples/flight)
  add_subdirectory(examples/hello_world)
  add_subdirectory(examples/combinations)
  add_subdirectory(examples/performance)
endif()
