#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

#include "asarBinding.h"
#include "Utility.h"
#include "Package.h"
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

TEST_CASE("Creating and reading files", "[utility][i-o]")
{
    const fs::path BIN_FILENAME = "bin.bin",
        TEXT_FILENAME = "txt.txt";

    std::vector<uint8_t> st_vec {1,2,3,4,5,6,7,8,9},
        blank_vec;
    writeBinaryFile(BIN_FILENAME, st_vec);
    readBinaryFile(BIN_FILENAME, blank_vec);

    REQUIRE(st_vec.size() == blank_vec.size());
    REQUIRE(fs::file_size(BIN_FILENAME) == st_vec.size());

    std::string st_str {"123456789\n"},
        blank_str;
    writeTextFile(TEXT_FILENAME, st_str);
    readTextFile(TEXT_FILENAME, blank_str);

    REQUIRE(st_str.size() == blank_str.size());
    REQUIRE(fs::file_size(TEXT_FILENAME) == st_str.size());
}

TEST_CASE("Testing the scanInt method", "[utility][scanint]")
{
    std::string test4scanint {R"(
        MainLoopPos: $00042E
        ReuploadPos: $00182A
    )"};

    uint32_t mainLoopPos, reuploadPos;
    mainLoopPos = scanInt(test4scanint, "MainLoopPos: ");
    reuploadPos = scanInt(test4scanint, "ReuploadPos: ");

    REQUIRE(mainLoopPos == 0x00042e);
    REQUIRE(reuploadPos == 0x00182a);
}

TEST_CASE("SPCEnvironment creation of a build environment", "[spcenvironment][instancing]")
{
    SPCEnvironment spc (WORK_DIR, DRIVER_DIR);
    REQUIRE(fs::exists(DRIVER_DIR / "build"));
}

TEST_CASE("Package extraction", "[package][extraction]")
{
    asm_package.extract(fs::path("extraction"));
    REQUIRE(true);
}

TEST_CASE("insertHexValue testing", "[utility][inserthexvalue]")
{
    std::string test4insertion {R"(
        Tag_1: 
        Tag_2: 
        Tag_3: 
        Tag_4: 
    )"};
    insertHexValue((uint8_t)0x42, "Tag_1", test4insertion);
    insertHexValue((uint16_t)0x42, "Tag_2", test4insertion);
    insertHexValue((uint32_t)0x42, "Tag_3", test4insertion);
    insertHexValue<uint32_t, 6>(0x42, "Tag_4", test4insertion);
    
    std::cout << test4insertion << std::endl;
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