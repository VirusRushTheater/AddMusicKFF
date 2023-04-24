cmake_minimum_required(VERSION 3.16)

project(AddMusicK
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK 1.0.8 branch by VirusRushTheater"
	LANGUAGES	C CXX
)

# The compiled program and driver source files will be end up being copied to this folder
set(PROJECT_DEPLOY_DIR				${CMAKE_BINARY_DIR}/deploy)
set(PROJECT_BUILD_SCRIPTS_DIR		${CMAKE_SOURCE_DIR}/build_scripts)

# Stuff that will be copied verbatim.
set(SNES_DRIVER_SRC					${CMAKE_SOURCE_DIR}/src/package/asm)
set(AMK_BOILERPLATE_SRC				${CMAKE_SOURCE_DIR}/src/package/boilerplate)
set(ADDMUSICMREMOVER_FILE			${CMAKE_SOURCE_DIR}/src/addmusicMRemover.pl)

# Set the output directories for libraries and executables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR})

# Defining AddMusicK compiling options
option(ADDMUSICK_COMPILE_AMKGUI
	"Compile AMKGUI (Supported only if you have Visual Studio available)"
OFF)
option(ADDMUSICK_COMPILE_AM4BATCH
	"Compile AM4Batch"
OFF)
option(ADDMUSICK_COMPILE_AMMBATCH
	"Compile AMMBatch"
OFF)
option(ADDMUSICK_COMPILE_TESTS
	"Compile tests"
ON)

if (ADDMUSICK_COMPILE_AMKGUI)
	message(FATAL_ERROR "No support yet for building AMKGUI.")
endif()

# Standardize this as much as possible.
set(CMAKE_CXX_STANDARD		17)
set(CMAKE_CXX_EXTENSIONS 	OFF)
# set(CMAKE_CXX_FLAGS 		"${CMAKE_CXX_FLAGS} -Wall")

set(ASAR_LIB_TARGET			asar-static)

# Branch the build to these directories
add_subdirectory(third-party)
add_subdirectory(src)

# Compile tests
if (ADDMUSICK_COMPILE_TESTS)
	# Detect Python, so we can test Addmusic environments with random sets
	# of songs.
	find_program(PYTHON_EXECUTABLE python)

	if(NOT PYTHON_EXECUTABLE)
		message(FATAL_ERROR "Python is needed to build tests. Set ADDMUSICK_COMPILE_TESTS to OFF if you don't want to build them.")
	endif()

	execute_process(
		COMMAND ${PYTHON_EXECUTABLE} "${PROJECT_BUILD_SCRIPTS_DIR}/amk_test_toolkit.py"
		WORKING_DIRECTORY "${PROJECT_BUILD_SCRIPTS_DIR}"
	)

	add_subdirectory(tests)
endif()

# Copy the SNES driver source to the deploy folder
file(COPY ${SNES_DRIVER_SRC} DESTINATION ${PROJECT_DEPLOY_DIR})
file(COPY ${AMK_BOILERPLATE_SRC} DESTINATION ${PROJECT_DEPLOY_DIR})
file(COPY ${ADDMUSICMREMOVER_FILE} DESTINATION ${PROJECT_DEPLOY_DIR})

message(STATUS "AddMusicK and the compiling environment will be stored in " ${PROJECT_DEPLOY_DIR})

# Copy the AddMusicK executable to the deploy folder
# set_target_properties(AddmusicK
# 	PROPERTIES
# 	ARCHIVE_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR}
# 	LIBRARY_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR}
# 	RUNTIME_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR}
# )

# And the Asar shared library as well.
# set_target_properties(${ASAR_LIB_TARGET}
# 	PROPERTIES
# 	LIBRARY_OUTPUT_DIRECTORY 	${PROJECT_DEPLOY_DIR}
# )

# After you run this CMake script, you should have a readily available
# "deploy" folder with all the programs and components necessary to run
# AddMusicK.

