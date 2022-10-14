cmake_minimum_required(VERSION 3.16)

project(AddMusicK
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK 1.0.8 branch by VirusRushTheater"
	LANGUAGES	C CXX
)

# Whatever results from compilation, will be copied here.
set(PROJECT_DEPLOY_DIR				${PROJECT_SOURCE_DIR}/deploy)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR})

# Defining AddMusicK compiling options
option(COMPILE_ASAR_LIBRARY
	"Transitory option to compile an Asar DLL to be used by AddmusicK"
OFF)
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
# set(CMAKE_CXX_FLAGS 		"${CMAKE_CXX_FLAGS} -Wall")

# Branch the build to these directories
add_subdirectory(third-party)
add_subdirectory(src)
