set(ADDMUSICK_SOURCES
	asarBinding.cpp
	Utility.cpp

	SPCEnvironment.cpp
	ROMEnvironment.cpp
	MMLBase.cpp
	Music.cpp
	SoundEffect.cpp

	experimental/MMLParserBase.cpp
)

set(ADDMUSICK_HEADERS
	AddmusicLogging.h
	asarBinding.h
	Utility.h
	Package.h

	SPCEnvironment.h
	ROMEnvironment.h
	MMLBase.h
	Music.h
	SoundEffect.h

	experimental/MMLParserBase.h
)

# Include Asar dependencies as explained in its git repository
get_target_property(ASAR_INCLUDE_DIR ${ASAR_LIB_TARGET} INCLUDE_DIRECTORIES)

# Used the .. directive in order to explicitly include <asar/interface-lib.h>
# instead of <interface-lib.h> for code readability.
include_directories(${ASAR_INCLUDE_DIR}/..)

# AddmusicK will act as a library. CLI and GUI will be built elsewhere.
add_library(${ADDMUSICKLIB_TARGETNAME} STATIC
	${ADDMUSICK_SOURCES}
	${ADDMUSICK_HEADERS}
)

# Set some project related constants in a generated defines.h in the build directory.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/defines.h.in "defines.h")

# Solution to have any application that uses this library to find the include
# files in this folder.
target_include_directories(${ADDMUSICKLIB_TARGETNAME} PUBLIC
	"./"
	"experimental"
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${ADDMUSICKLIB_TARGETNAME}
	AM405Remover
)