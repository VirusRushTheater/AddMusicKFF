set(AM405REMOVER_SOURCES
	AddMusic.cpp
)
set(AM405REMOVER_HEADERS
	AddMusic.h
	AM405Remover.h
)

add_library(AM405Remover
	STATIC
	${AM405REMOVER_HEADERS}
	${AM405REMOVER_SOURCES}
)

target_include_directories(AM405Remover
	PUBLIC
	"./"
)