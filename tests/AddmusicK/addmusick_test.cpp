#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <type_traits>

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

    uint24_t mainLoopPos, reuploadPos;
    mainLoopPos = scanInt(test4scanint, "MainLoopPos: ");
    reuploadPos = scanInt(test4scanint, "ReuploadPos: ");

    REQUIRE(mainLoopPos == 0x00042e);
    REQUIRE(reuploadPos == 0x00182a);
}

// TEST_CASE("SPCEnvironment creation of a build environment", "[spcenvironment][instancing]")
// {
//     SPCEnvironment spc (WORK_DIR, DRIVER_DIR);
//     REQUIRE(fs::exists(DRIVER_DIR / "build"));
// }

// TEST_CASE("Package extraction", "[package][extraction]")
// {
//     asm_package.extract(fs::path("extraction"));
//     REQUIRE(true);
// }

TEST_CASE("replaceHexValue testing", "[utility][replacehexvalue]")
{
    std::string test4insertion 
    {R"(!ExpARAMRet = $0400
        !DefARAMRet = $042F
        !SongCount = $00
        !GlobalMusicCount = #$00

        !uint8_t = $01
        !uint16_t = $01
        !uint24_t = $01
        !uint32_t = $01)"};
    
    replaceHexValue(0x01_u16, "!ExpARAMRet = ", test4insertion);
    replaceHexValue(0x02_u16, "!DefARAMRet = ", test4insertion);
    replaceHexValue(0x03_u8, "!SongCount = ", test4insertion);
    replaceHexValue(0x04_u8, "!GlobalMusicCount = #", test4insertion);
    
    replaceHexValue(0x05_u8, "!uint8_t = ", test4insertion);
    replaceHexValue(0x06_u16, "!uint16_t = ", test4insertion);
    replaceHexValue(0x07_u24, "!uint24_t = ", test4insertion);
    replaceHexValue(0x08_u32, "!uint32_t = ", test4insertion);
    
    REQUIRE (test4insertion == 
     R"(!ExpARAMRet = $0001
        !DefARAMRet = $0002
        !SongCount = $03
        !GlobalMusicCount = #$04

        !uint8_t = $05
        !uint16_t = $0006
        !uint24_t = $000007
        !uint32_t = $00000008)");
}

TEST_CASE("hexDump testing", "[utility][hexdump]")
{
    std::string
        t_8 = hexDump(0x25_u8),
        t_16 = hexDump(0x25_u16),
        t_24 = hexDump(0x25_u24),
        t_32 = hexDump(0x25_u32);
    
    REQUIRE(t_8 == "25");
    REQUIRE(t_16 == "0025");
    REQUIRE(t_24 == "000025");
    REQUIRE(t_32 == "00000025");
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

    SPCEnvironment spc (TEST_WORKDIR);
    spc.generateSPCFiles(input_files);

}