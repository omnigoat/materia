cmake_minimum_required(VERSION 3.31)

# ignore these variables set for dev purposes
set(ignore "${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}${CMAKE_C_COMPILER}")

# set global language standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# materia umbrella project
project(materia LANGUAGES CXX)

# msvc-specific settings
set(MATERIA_CPP_FLAGS $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

# include projects
add_subdirectory(projects)
