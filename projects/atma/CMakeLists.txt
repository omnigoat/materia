cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(atma STATIC)

# includes
file(GLOB_RECURSE include_set include/atma/*.hpp)
target_include_directories(atma PUBLIC include)
target_sources(atma PUBLIC
    ${include_set})

# sources
file(GLOB_RECURSE source_set src/**/*.cpp)
target_sources(atma PRIVATE
    ${source_set})

# modules
file(GLOB module_set modules/*.cppm)
target_sources(atma PUBLIC
    FILE_SET modules
    TYPE CXX_MODULES
    FILES ${module_set})


source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/atma PREFIX "include" FILES ${include_set})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_set} ${module_set})

target_link_libraries(atma PUBLIC doctest)
target_link_libraries(atma PUBLIC boost)
