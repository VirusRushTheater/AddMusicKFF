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

if (ADDMUSICK_CREATE_EXECUTABLE)
	add_executable(AddmusicK
		${ADDMUSICK_HEADERS}
		${ADDMUSICK_SOURCES}
	)

	target_link_libraries(AddmusicK PUBLIC
		AM405Remover
	)

	# target_include_directories(AddmusicK PUBLIC
	# 	"./"
	# )
endif()