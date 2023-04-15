# find_package(Catch2 REQUIRED)

add_executable(tests
	asarBinding_test.cpp
)

target_include_directories(tests PUBLIC
    ./
)

target_link_libraries(tests PUBLIC
    AddmusicK
    Catch2::Catch2
)
