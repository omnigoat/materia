cmake_minimum_required(VERSION 3.31)
set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(atma STATIC)

target_compile_options(atma PRIVATE
	/Zc:preprocessor)

# includes
target_include_directories(atma PUBLIC include)

target_sources(atma 
	PUBLIC FILE_SET headers
	TYPE HEADERS
	BASE_DIRS include/atma FILES
		include/atma/config/platform.hpp
		include/atma/math/aabb.hpp
		include/atma/math/aabc.hpp
		include/atma/math/constants.hpp
		include/atma/math/functions.hpp
		include/atma/math/intersection.hpp
		include/atma/math/matrix4f.hpp
		include/atma/math/scalar.hpp
		include/atma/math/triangle.hpp
		include/atma/math/vector4f.hpp
		include/atma/math/vector4i.hpp
		include/atma/platform/allocation.hpp
		include/atma/platform/interop.hpp
		include/atma/ranges/core.hpp
		include/atma/ranges/filter.hpp
		include/atma/ranges/iota.hpp
		include/atma/ranges/map.hpp
		include/atma/ranges/zip.hpp
		include/atma/thread/engine.hpp
		include/atma/utf/algorithm.hpp
		include/atma/utf/utf8_string.hpp
		include/atma/algorithm.hpp
		include/atma/arena_allocator.hpp
		include/atma/assert.hpp
		include/atma/atomic.hpp
		include/atma/benchmark.hpp
		include/atma/bitmask.hpp
		include/atma/com_ptr.hpp
		include/atma/console.hpp
		include/atma/delegate.hpp
		include/atma/ebo_pair.hpp
		include/atma/event.hpp
		include/atma/flyweight.hpp
		include/atma/function.hpp
		include/atma/function_composition.hpp
		include/atma/functor.hpp
		include/atma/handle_table.hpp
		include/atma/hash.hpp
		include/atma/hash_map.hpp
		include/atma/lockfree_list.hpp
		include/atma/lockfree_queue.hpp
		include/atma/logging.hpp
		include/atma/preprocessor.hpp
		include/atma/rope.hpp
		include/atma/shared_memory.hpp
		include/atma/streams.hpp
		include/atma/string.hpp
		include/atma/threading.hpp
		include/atma/tuple.hpp
		include/atma/unit_test.hpp)

# sources
set(source_set
	src/atma_test/bind_and_function.cpp
	src/atma_test/entrypoint.cpp
	src/atma_test/memory.cpp
	src/atma_test/queue.cpp
	src/atma_test/ranges.cpp
	src/atma_test/test_allocators.cpp
	src/atma_test/test_atomics.cpp
	src/atma_test/test_events.cpp
	src/atma_test/test_pointers.cpp
	src/atma_test/test_rope.cpp
	src/atma_test/test_utf8_string.cpp
	src/atma_test/test_vector.cpp)

# modules
target_sources(atma PUBLIC
	FILE_SET modules
	TYPE CXX_MODULES
	FILES 
		modules/aligned_allocator.cppm
		modules/assert.cppm
		modules/atomic.cppm
		modules/benchmark.cppm
		modules/bind.cppm
		modules/intrusive_ptr.cppm
		modules/lockfree_queue.cppm
		modules/memory.cppm
		modules/meta.cppm
		modules/types.cppm
		modules/vector.cppm)


#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/atma PREFIX "include" FILES ${headers})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_set} ${modules})

target_link_libraries(atma PUBLIC doctest)
target_link_libraries(atma PUBLIC boost)



#
# atma_test
# ----------
#
add_executable(atma_test)

target_sources(atma_test PRIVATE
	src/atma_test/bind_and_function.cpp
	src/atma_test/entrypoint.cpp
	src/atma_test/memory.cpp
	src/atma_test/queue.cpp
	src/atma_test/ranges.cpp
	src/atma_test/test_allocators.cpp
	src/atma_test/test_atomics.cpp
	src/atma_test/test_events.cpp
	src/atma_test/test_pointers.cpp
	src/atma_test/test_rope.cpp
	src/atma_test/test_utf8_string.cpp
	src/atma_test/test_vector.cpp)

target_compile_options(atma_test PRIVATE
	/Zc:preprocessor)

target_link_libraries(atma_test PUBLIC atma)