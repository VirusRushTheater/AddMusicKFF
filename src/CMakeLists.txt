add_subdirectory(AM405Remover)
add_subdirectory(AddmusicK)

# Controlled by the options in the root CMakeLists.
# Currently, C# is only supported by Visual Studio pipelines, not Mono.
# I hope this changes soon.
if(ADDMUSICK_COMPILE_AMKGUI)
	if (NOT DEFINED CMAKE_CSharp_COMPILER)
		message(WARNING "C# compiling system not found! AMKGUI won't be built.")
		message("Set ADDMUSICK_COMPILE_AMKGUI to OFF to turn off this warning.")
	else ()
		message(STATUS "AMKGUI set to build.")
		add_subdirectory(AMKGUI)
	endif ()
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