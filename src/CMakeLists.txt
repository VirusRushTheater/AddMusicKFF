add_subdirectory(AM405Remover)
add_subdirectory(AddmusicK)

# Controlled by the options in the root CMakeLists.
if(ADDMUSICK_COMPILE_AMKGUI)
	message(STATUS "AMKGUI set to build.")
	add_subdirectory(AMKGUI)
else()
	message(STATUS "AMKGUI set to NOT be built.")
endif(ADDMUSICK_COMPILE_AMKGUI)

# Directories that don't make anything but they are there for some reason.
if(ADDMUSICK_COMPILE_AM4BATCH)
	add_subdirectory(AM4Batch)
	message(STATUS "AM4Batch set to build.")
else()
	message(STATUS "AM4Batch set to NOT be built.")
endif(ADDMUSICK_COMPILE_AM4BATCH)

# AM4Batch
if(ADDMUSICK_COMPILE_AMMBATCH)
	add_subdirectory(AMMBatch)
	message(STATUS "AMMBatch set to build.")
else()
	message(STATUS "AMMBatch set to NOT be built.")
endif(ADDMUSICK_COMPILE_AMMBATCH)
	
# Generate resource files out of every folder in "package".
add_subdirectory(package)

add_subdirectory(AMKCLI)