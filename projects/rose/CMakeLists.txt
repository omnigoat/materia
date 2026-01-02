cmake_minimum_required(VERSION 3.31)

add_library(rose STATIC)

# compile options
target_compile_options(rose PRIVATE ${MATERIA_CPP_FLAGS})

# dependencies
target_link_libraries(rose PUBLIC atma)

# include directories
target_include_directories(rose
	PUBLIC include)

# sources
target_sources(rose
	PUBLIC
		include/rose/console.hpp
		include/rose/file.hpp
		include/rose/mmap.hpp
		include/rose/path.hpp
		include/rose/rose_fwd.hpp
		include/rose/runtime.hpp
	PRIVATE
		src/runtime.cpp
		src/windows/console.cpp
		src/windows/file.cpp
		src/windows/mmap.cpp)
