cmake_minimum_required(VERSION 3.16)

project(AddMusicK
	VERSION		1.0.8
	DESCRIPTION	"AddmusicK 1.0.8 branch by VirusRushTheater"
	LANGUAGES	C CXX
)

# Whatever results from compilation, will be copied here.
set(PROJECT_DEPLOY_DIR				${PROJECT_SOURCE_DIR}/deploy)

# Defining AddMusicK compiling options
option(ADDMUSICK_COMPILE_AMKGUI
	"Compile AMKGUI (Supported only if you have Visual Studio available)"
OFF)
option(ADDMUSICK_COMPILE_AM4BATCH
	"Compile AM4Batch"
ON)
option(ADDMUSICK_COMPILE_AMMBATCH
	"Compile AMMBatch"
ON)

if (ADDMUSICK_COMPILE_AMKGUI)
	message(FATAL_ERROR "No support yet for building AMKGUI.")
endif()

# Standardize this as much as possible.
set(CMAKE_CXX_STANDARD		17)
set(CMAKE_CXX_EXTENSIONS 	OFF)
# set(CMAKE_CXX_FLAGS 		"${CMAKE_CXX_FLAGS} -Wall")

set(ASAR_LIB_TARGET			asar)

# Branch the build to these directories
add_subdirectory(third-party)
add_subdirectory(src)
add_subdirectory(tests)

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
