cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 20)

add_library(boost INTERFACE)

# includes
target_include_directories(boost INTERFACE include)

