cmake_minimum_required(VERSION 3.31)

add_library(rose STATIC)

# compile options
target_compile_options(rose PRIVATE ${MATERIA_CPP_FLAGS})

# dependencies
target_link_libraries(rose PUBLIC atma)

# include directories
target_include_directories(rose
	PUBLIC include)
	
# common
target_sources(rose PUBLIC
	FILE_SET modules
	TYPE CXX_MODULES
	FILES
		src/rose.cppm
		src/@common/runtime.cppm
		src/@common/console.cppm
		src/@common/file.cppm
		src/@common/path.cppm
		src/@common/mmap.cppm)

# windows
target_sources(rose PUBLIC
	FILE_SET modules
	TYPE CXX_MODULES
	FILES 
		src/windows/console.cppm
		src/windows/file.cppm
		src/windows/mmap.cppm
		src/windows/runtime.cppm)


# sources
target_sources(rose
	PUBLIC
		include/rose/rose_fwd.hpp)


