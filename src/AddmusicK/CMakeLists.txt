set(ADDMUSICK_SOURCES
	asardll.c
	AddmusicK.cpp
	globals.cpp
	lodepng.cpp
	Music.cpp
	SoundEffect.cpp
)

set(ADDMUSICK_HEADERS
	asardll.h
	BankDefine.h
	Directory.h
	fs.h
	globals.h
	lodepng.h
	Music.h
	SampleGroup.h
	Sample.h
	SoundEffect.h
)

# Include Asar dependencies as explained in its git repository
get_target_property(ASAR_INCLUDE_DIR asar-static INCLUDE_DIRECTORIES)
include_directories(${ASAR_INCLUDE_DIR})

if (ADDMUSICK_CREATE_EXECUTABLE)
	add_executable(AddmusicK
		${ADDMUSICK_HEADERS}
		${ADDMUSICK_SOURCES}
	)

	# Required AM405Remover (previously built as a static library)
	# and asar as a library.
	target_link_libraries(AddmusicK PUBLIC
		AM405Remover
		asar-static
	)
endif()

if (ADDMUSICK_CREATE_SHARED_LIB)
	message("Compilation of AddmusicK as a library is not yet supported")
endif()

if (ADDMUSICK_CREATE_STATIC_LIB)
	message("Compilation of AddmusicK as a library is not yet supported")
endif()