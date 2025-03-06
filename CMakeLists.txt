cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-g")

project(mipt-comp-math)
include_directories(Common/Inc)

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(1-HelloWorld 1-Intro/Src/HelloWorld.cpp)
target_link_libraries(1-HelloWorld PRIVATE MPI::MPI_CXX)

add_executable(1-SeriesSum 1-Intro/Src/SeriesSum.cpp)
target_link_libraries(1-SeriesSum PRIVATE MPI::MPI_CXX)

add_executable(1-DutchWheel 1-Intro/Src/DutchWheel.cpp)
target_link_libraries(1-DutchWheel PRIVATE MPI::MPI_CXX)