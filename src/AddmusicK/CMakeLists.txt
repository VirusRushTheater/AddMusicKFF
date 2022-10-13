set(ADDMUSICK_SOURCES
	AddmusicK.cpp
	asardll.c
	globals.cpp
	lodepng.cpp
	Music.cpp
	SoundEffect.cpp
)

add_library(AddmusicK
	SHARED
	${ADDMUSICK_SOURCES}
)

target_include_directories(AddmusicK PUBLIC
	"./"
)