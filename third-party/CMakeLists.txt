# Command Asar to be compiled only as a static library
# Also, hardcode these options.

if (COMPILE_ASAR_LIBRARY)
	set(ASAR_GEN_EXE 			OFF CACHE BOOL "" FORCE)
	set(ASAR_GEN_DLL 			ON  CACHE BOOL "" FORCE)
	set(ASAR_GEN_EXE_TEST 		OFF CACHE BOOL "" FORCE)
	set(ASAR_GEN_DLL_TEST 		OFF CACHE BOOL "" FORCE)
	set(ASAR_GEN_LIB 			OFF CACHE BOOL "" FORCE)

	add_subdirectory(asar/src)
endif(COMPILE_ASAR_LIBRARY)