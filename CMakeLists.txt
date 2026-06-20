cmake_minimum_required(VERSION 3.31)

# ignore these variables set for dev purposes
set(ignore "${CMAKE_SUPPRESS_DEVELOPER_WARNINGS}${CMAKE_C_COMPILER}")

# set global language standard
set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_MODULE_STD 1)
#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(MATERIA_CACHE_DIR "${CMAKE_SOURCE_DIR}/.cmake_cache")
set(FETCHCONTENT_BASE_DIR "${MATERIA_CACHE_DIR}/deps")

#
# CPM.cmake
# -----------
#
set(CPM_DOWNLOAD_VERSION 0.42.3) 
set(CPM_DOWNLOAD_LOCATION "${MATERIA_CACHE_DIR}/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
	message(STATUS "Downloading CPM.cmake")
	file(DOWNLOAD 
		https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
		${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})


#
# materia umbrella project
# ----------------------------
#
project(materia LANGUAGES CXX)

# msvc-specific settings
set(MATERIA_CPP_FLAGS $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor /D_HAS_EXCEPTIONS=0>)

# include projects
add_subdirectory(projects)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT atma_test)
