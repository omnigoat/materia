cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 20)

project(materia)

set(MATERIA_CPP_FLAGS
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

add_subdirectory(projects/doctest doctest)
add_subdirectory(projects/boost boost)
add_subdirectory(projects/atma atma)
add_subdirectory(projects/rose rose)

set_property(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT rose)
