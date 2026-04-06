cmake_minimum_required(VERSION 3.31)

add_executable(mantle)

# project properties
set_target_properties(mantle PROPERTIES
	VS_GLOBAL_Nullable "enable"
	VS_GLOBAL_ImplicitUsings "enable"
	VS_GLOBAL_UseAppHost "true"
	VS_PACKAGE_REFERENCES "System.CommandLine_2.0.5;Microsoft.CodeAnalysis_5.3.0")

# source files
target_sources(mantle
	PRIVATE
		src/Mantle.cs
		src/Dyncom.cs
		src/Commands/Gen.cs)

# set up visual studio filters
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
	src/Mantle.cs
	src/Dyncom.cs
	src/Commands/Gen.cs)