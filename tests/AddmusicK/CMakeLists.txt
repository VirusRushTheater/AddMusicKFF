# find_package(Catch2 REQUIRED)

add_executable(test_units
	addmusick_test.cpp
)

target_include_directories(test_units PUBLIC
    ./
)

target_link_libraries(test_units PUBLIC
    ${ADDMUSICKLIB_TARGETNAME}
    Catch2::Catch2
    ${ASAR_LIB_TARGET}
)
