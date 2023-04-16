#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

#include "asarBinding.h"
#include "Utility.h"

using namespace AddMusic;
namespace fs = std::filesystem;

const fs::path DRIVER_DIR {"../snes-driver"};

TEST_CASE("asarBinding compilation to memory", "[addmusick][asarbinding][compilation]")
{
    AsarBinding asar (DRIVER_DIR / "asm" / "main.asm");
    bool success = asar.compileToBin();
    std::cerr << "== stderr from main.asm compilation ==" << std::endl << asar.getStderr() << std::endl;
    std::cerr << "== stdout from main.asm compilation ==" << std::endl << asar.getStdout() << std::endl;
    REQUIRE(success);
}