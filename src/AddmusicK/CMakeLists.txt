set(ADDMUSICK_SOURCES
	asarBinding.cpp
	Utility.cpp
	Package.cpp

	SPCEnvironment.cpp
	# ROMEnvironment.cpp
	MMLBase.cpp
	Music.cpp
	SoundEffect.cpp
)

set(ADDMUSICK_HEADERS
	AddmusicLogging.h
	asarBinding.h
	defines.h
	BankDefine.h
	Sample.h
	Utility.h
	Package.h

	SPCEnvironment.h
	# ROMEnvironment.h
	MMLBase.h
	Music.h
	SoundEffect.h
)

# Include Asar dependencies as explained in its git repository
get_target_property(ASAR_INCLUDE_DIR ${ASAR_LIB_TARGET} INCLUDE_DIRECTORIES)

# Used the .. directive in order to explicitly include <asar/interface-lib.h>
# instead of <interface-lib.h> for code readability.
include_directories(${ASAR_INCLUDE_DIR}/..)

# AddmusicK will act as a library. CLI and GUI will be built elsewhere.
add_library(AddmusicK STATIC
	${ADDMUSICK_SOURCES}
	${ADDMUSICK_HEADERS}
)

# Solution to have any application that uses this library to find the include
# files in this folder.
target_include_directories(AddmusicK PUBLIC "./")

target_link_libraries(AddmusicK
	AM405Remover
)