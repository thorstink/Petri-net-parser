
cmake_minimum_required(VERSION 3.10)
project(Symmetri VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer -Wno-psabi -Wno-unused-parameter -Werror -Wall -Wextra -march=native -O2 -s")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -fno-omit-frame-pointer -Wno-psabi -Wno-unused-parameter -Werror -Wall -Wextra -march=native -O2 -s")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-psabi -Wno-unused-parameter -Werror -Wall -Wextra -march=native -O2 -s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-psabi -Wno-unused-parameter -Werror -Wall -Wextra -march=native -O2 -s")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wno-psabi  -march=native -O2 -s")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-psabi -march=native -O2 -s")

find_package(Eigen3 REQUIRED)
find_package(Seasocks REQUIRED)

include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

include(FetchContent)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY    https://github.com/cameron314/concurrentqueue
  GIT_TAG           master
)

FetchContent_MakeAvailable(concurrentqueue)

add_subdirectory(submodules/MPMCQueue MPMCQueue)

include_directories(${EIGEN3_INCLUDE_DIRS} ${SEASOCKS_INCLUDE_DIRS} submodules .)

add_library(${PROJECT_NAME}_pnml_parser Symmetri/pnml_parser.cc submodules/tinyxml2/tinyxml2.cpp)

#lib
add_library(lib${PROJECT_NAME} Symmetri/symmetri.cc Symmetri/model.cc Symmetri/actions.cc)
target_link_libraries(lib${PROJECT_NAME} pthread Seasocks::seasocks ${PROJECT_NAME}_pnml_parser concurrentqueue)

#an app
add_executable(${PROJECT_NAME}_app application/main.cc application/transitions.cc)
target_link_libraries(${PROJECT_NAME}_app lib${PROJECT_NAME})

set(ui_files
  ${CMAKE_CURRENT_SOURCE_DIR}/Symmetri/web
)
set(targets
  ${PROJECT_NAME}_app
)

install(DIRECTORY ${ui_files} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)
install(TARGETS ${targets} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)

# if(BUILD_TESTING)
# Enable testing for the project
enable_testing()
# concat testfiles 
set(TEST_SOURCES
catch.cc
tests.cc
)

# add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
# add_test(${PROJECT_NAME}_test ${PROJECT_NAME}_test)
# endif()