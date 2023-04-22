#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

#include "asarBinding.h"
#include "Utility.h"
#include "SPCEnvironment.h"

using namespace AddMusic;
namespace fs = std::filesystem;

const fs::path WORK_DIR {"../boilerplate"};
const fs::path DRIVER_DIR {"../asm"};

TEST_CASE("asarBinding compilation to memory", "[addmusick][asarbinding][compilation]")
{
    AsarBinding asar (DRIVER_DIR / "asm" / "main.asm");
    bool success = asar.compileToBin();
    std::cerr << "== stderr from main.asm compilation ==" << std::endl << asar.getStderr() << std::endl;
    std::cerr << "== stdout from main.asm compilation ==" << std::endl << asar.getStdout() << std::endl;
    REQUIRE(success);
}

TEST_CASE("Logging", "[logging]")
{
    Logging::debug("Debug (this should not be printed yet)");
    Logging::info("Info");
    Logging::warning("Warning");

    Logging::setVerbosity(Logging::Levels::DEBUG);
    Logging::debug("Debug (after setting verbose mode)");

    Logging::setVerbosity(Logging::Levels::INFO);
    Logging::debug("Debug (this should not be printed)");
}

TEST_CASE("SPCEnvironment creation of a build environment", "[spcenvironment][instancing]")
{
    SPCEnvironment spc (WORK_DIR, DRIVER_DIR);
    REQUIRE(fs::exists(DRIVER_DIR / "build"));
}