cmake_minimum_required(VERSION 3.31)

# downloaded dependencies
CPMAddPackage(
	NAME doctest
	GIT_REPOSITORY "https://github.com/doctest/doctest"
	GIT_TAG "v2.5.1"
	OPTIONS
		"DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF")

add_subdirectory(boost)
add_subdirectory(atma)
add_subdirectory(rose)

