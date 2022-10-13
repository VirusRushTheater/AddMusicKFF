cmake_minimum_required(VERSION 3.16)

project(AddMusicK
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK 1.0.8 branch by VirusRushTheater"
	LANGUAGES	C CXX
)

# Defining AddMusicK compiling options
option(ADDMUSICK_CREATE_EXECUTABLE
	"Compile AddMusicK as an executable"
ON)
option(ADDMUSICK_CREATE_SHARED_LIB
	"Compile AddMusicK as a shared library (TODO)"
OFF)
option(ADDMUSICK_CREATE_STATIC_LIB
	"Compile AddMusicK as a static library (TODO)"
OFF)

# Standardize this as much as possible.
set(CMAKE_CXX_STANDARD		17)
set(CMAKE_CXX_EXTENSIONS 	OFF)

# Branch the build to these directories
add_subdirectory(third-party)
add_subdirectory(src)