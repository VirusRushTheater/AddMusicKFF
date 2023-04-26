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
const fs::path TEST_WORKDIR {"../random_env"};

const fs::path DRIVER_DIR {"../asm"};

TEST_CASE("asarBinding compilation to memory", "[addmusick][asarbinding][compilation]")
{
    AsarBinding asar (DRIVER_DIR / "main.asm");
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

    REQUIRE(true);
}

TEST_CASE("SPCEnvironment creation of a build environment", "[spcenvironment][instancing]")
{
    SPCEnvironment spc (WORK_DIR, DRIVER_DIR);
    REQUIRE(fs::exists(DRIVER_DIR / "build"));
}

TEST_CASE("SPCEnvironment creation of a set of SPC files", "[spcenvironment][spc][generation]")
{
    const fs::path testset = TEST_WORKDIR / "music";
    std::vector<fs::path> input_files;

    // Scans the Music folder for new .txt files.
    for (auto& file_i : fs::directory_iterator(testset))
    {
        if (file_i.path().extension().string() == ".txt")
            input_files.push_back(fs::absolute(file_i.path()));
    }

    SPCEnvironment spc (TEST_WORKDIR, DRIVER_DIR);
    spc.generateSPCFiles(input_files);

}