cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(rose STATIC)

add_dependencies(rose atma)

# includes
set(include_set
    include/rose/console.hpp
    include/rose/file.hpp
    include/rose/mmap.hpp
    include/rose/path.hpp
    include/rose/rose_fwd.hpp
    include/rose/runtime.hpp)

target_include_directories(rose PUBLIC include)
target_sources(rose PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include/rose
    FILES
    ${include_set})

# sources
set(source_set
    src/runtime.cpp
    src/windows/console.cpp
    src/windows/file.cpp
    src/windows/mmap.cpp)

target_sources(rose PRIVATE ${source_set})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/rose PREFIX "include" FILES ${include_set})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_set})
