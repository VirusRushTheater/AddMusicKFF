# find_package(Catch2 REQUIRED)

add_executable(test_units
	addmusick_test.cpp
)

target_include_directories(test_units PUBLIC
    ./
    ${CMAKE_SOURCE_DIR}/third-party/Catch2/src
)

target_link_libraries(test_units PUBLIC
    ${ADDMUSICKLIB_TARGETNAME}
    Catch2::Catch2WithMain
    ${ASAR_LIB_TARGET}
)

# Enable testing and add the test
enable_testing()
add_test(NAME AddmusicK_unit_tests COMMAND test_units)
