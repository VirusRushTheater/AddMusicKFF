# Command Asar to be compiled only as a static library
# Also, hardcode these options.

set(ASAR_GEN_EXE 			OFF CACHE BOOL "" FORCE)
set(ASAR_GEN_EXE_TEST 		OFF CACHE BOOL "" FORCE)
set(ASAR_GEN_DLL_TEST 		OFF CACHE BOOL "" FORCE)

if (ASAR_DYNAMIC_LINKAGE)
	set(ASAR_GEN_DLL 			ON CACHE BOOL "" FORCE)
	set(ASAR_GEN_LIB 			OFF  CACHE BOOL "" FORCE)
else()
	set(ASAR_GEN_DLL 			OFF CACHE BOOL "" FORCE)
	set(ASAR_GEN_LIB 			ON  CACHE BOOL "" FORCE)
endif()

add_subdirectory(asar/src)
add_subdirectory(Catch2)
add_subdirectory(cxxopts)

# Asar uses an address sanitizer, which makes the linker complain a lot when
# you compile AddMusic with debugging symbols.
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(${ASAR_LIB_TARGET} asan)
endif()
