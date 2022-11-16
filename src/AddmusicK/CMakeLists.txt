set(ADDMUSICK_SOURCES
	AddmusicK.cpp
	lodepng.cpp
	Music.cpp
	SoundEffect.cpp
	Utility.cpp
	logging.cpp
	AddmusicK_CLI.cpp
)

set(ADDMUSICK_HEADERS
	AddmusicK.hpp
	logging.hpp
	BankDefine.h
	Directory.h
	fs.h
	lodepng.h
	Music.h
	SampleGroup.h
	Sample.h
	SoundEffect.h
	Utility.h
)

# Include Asar dependencies as explained in its git repository
get_target_property(ASAR_INCLUDE_DIR ${ASAR_LIB_TARGET} INCLUDE_DIRECTORIES)

# Used the .. directive in order to explicitly include <asar/interface-lib.h>
# instead of <interface-lib.h> for code readability.
include_directories(${ASAR_INCLUDE_DIR}/..)

if (ADDMUSICK_BUILD_EXECUTABLE)
	message(STATUS "AddMusicK set to be built as an executable.")
	add_executable(AddmusicK
		${ADDMUSICK_HEADERS}
		${ADDMUSICK_SOURCES}
	)

	# Required AM405Remover (previously built as a static library)
	# and asar as a library.
	target_link_libraries(AddmusicK PUBLIC
		AM405Remover

		${ASAR_LIB_TARGET}
		cxxopts		# Command line libraries
	)

	target_include_directories(AddmusicK PUBLIC
		${ASAR_INCLUDE_DIR}/..
	)

elseif (ADDMUSICK_BUILD_LIBRARY)
	message(STATUS "AddMusicK set to be built as a library.")
	add_library(AddmusicK)
endif ()