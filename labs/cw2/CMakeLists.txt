project(set10108-cw02 LANGUAGES CXX)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)

#### OPEN MP PACKAGES ####
find_package(OpenMP)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

file(GLOB_RECURSE SOURCE_FILES lib_nqueens_bit ./lib_nqueens_bit/*.cpp ./lib_nqueens_bit/*.h)
add_library(lib_nqueens_bit STATIC ${SOURCE_FILES})
target_include_directories(lib_nqueens_bit INTERFACE "${CMAKE_SOURCE_DIR}/lib_nqueens_bit/")
target_link_libraries(lib_nqueens_bit) 

add_executable(NQueens nqueens.cpp)
target_link_libraries(NQueens lib_nqueens_bit)