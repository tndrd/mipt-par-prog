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

add_executable(2-Admission 2-conv-diff/Src/Admission.cpp)
target_link_libraries(2-Admission PRIVATE MPI::MPI_CXX)

add_executable(2-Task 2-conv-diff/Src/Task.cpp)
target_include_directories(2-Task PRIVATE 2-conv-diff/Inc)
target_link_libraries(2-Task PRIVATE MPI::MPI_CXX)

add_executable(3-HelloWorld 3-pthread-intro/Src/3-HelloWorld.cpp)
target_link_libraries(3-HelloWorld PRIVATE pthread)

add_executable(3-SeriesSum 3-pthread-intro/Src/3-SeriesSum.cpp)
target_link_libraries(3-SeriesSum PRIVATE pthread)

add_executable(3-SequentialAccess 3-pthread-intro/Src/3-SequentialAccess.cpp)
target_link_libraries(3-SequentialAccess PRIVATE pthread)