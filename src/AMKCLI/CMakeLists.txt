set(AMKCLI_SOURCES
	main.cpp
)

set(AMKCLI_HEADERS
)

add_executable(${ADDMUSICKCLI_TARGETNAME}
	${AMKCLI_SOURCES}
	${AMKCLI_HEADERS}
)

target_include_directories(${ADDMUSICKCLI_TARGETNAME} PUBLIC
    ./
)

target_link_libraries(${ADDMUSICKCLI_TARGETNAME} PUBLIC
	${ADDMUSICKLIB_TARGETNAME}
	${ASAR_LIB_TARGET}
	cxxopts
)

# # Install the executable to the bin directory
# install(TARGETS MyExecutable DESTINATION bin)