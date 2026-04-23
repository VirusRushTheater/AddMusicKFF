#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <cctype>

#include "asarBinding.h"
#include "Utility.h"
#include "Package.h"
#include "SPCEnvironment.h"
#include "ROMEnvironment.h"
#include "SMWTestROMGenerator.h"

using namespace AddMusic;
namespace fs = std::filesystem;

const fs::path getSmwRomPath()
{
    const char ENV_VAR_NAME[] = "SMW_ROM_LOCATION";
    fs::path empty_rom{};

    // Option 1: Check environment variable
    const char* env_path = std::getenv(ENV_VAR_NAME);
    if (env_path && *env_path)
        if (fs::exists(env_path))
            return fs::path(env_path);
    
    // Option 2: Check common filenames in the current directory
    std::vector<fs::path> possible_smw_paths = {
        "smw.smc",
        "smw_clean.smc",
        "Super Mario World.smc",
        "Super Mario World (U) [!].smc",
    };

    for (const auto& path : possible_smw_paths) {
        if (fs::exists(path)) {
            return path;
        }
    }

    // If we reach this point, no ROM was found
    return empty_rom;
}

TEST_CASE("The Logging class", "[logging]")
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

TEST_CASE("Creating and reading files with vectors using AddMusic's writeBinaryFile, readBinaryFile, writeTextFile and readTextFile", "[utility][i-o]")
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

    fs::remove(BIN_FILENAME);
    fs::remove(TEXT_FILENAME);
}

TEST_CASE("Testing the scanInt method", "[utility][driver][scanint]")
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

TEST_CASE("replaceHexValue testing. Used on replacing values onto the ASM drivers.", "[utility][driver][replacehexvalue]")
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

TEST_CASE("hexDump testing. Used on dumping hexadecimal values onto the ASM drivers.", "[utility][driver][hexdump]")
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

TEST_CASE("Boilerplate package extraction", "[package][extraction][boilerplate]")
{
    // Create a temporary directory for extraction
    fs::path temp_dir = fs::temp_directory_path() / "amk_test_boilerplate";
    fs::remove_all(temp_dir); // Clean up any previous test runs

    // Extract the boilerplate package
    bool success = AddMusic::boilerplate_package.extract(temp_dir);
    REQUIRE(success);

    // Verify the extraction directory was created
    REQUIRE(fs::exists(temp_dir));
    REQUIRE(fs::is_directory(temp_dir));

    // Check for expected boilerplate files
    REQUIRE(fs::exists(temp_dir / "Addmusic_list.txt"));
    REQUIRE(fs::exists(temp_dir / "Addmusic_sample groups.txt"));
    REQUIRE(fs::exists(temp_dir / "Addmusic_sound effects.txt"));

    // Check for expected subdirectories
    REQUIRE(fs::exists(temp_dir / "1DF9"));
    REQUIRE(fs::exists(temp_dir / "1DFC"));
    REQUIRE(fs::exists(temp_dir / "music"));
    REQUIRE(fs::exists(temp_dir / "samples"));

    // Verify at least one file in each subdirectory (basic smoke test)
    REQUIRE(!fs::is_empty(temp_dir / "music"));
    REQUIRE(!fs::is_empty(temp_dir / "samples"));

    // Clean up
    fs::remove_all(temp_dir);
}

TEST_CASE("ASM driver package extraction and compilation to memory", "[package][extraction][asm]")
{
    // === STEP 1: Extract the ASM package ===
    // Create a temporary directory for extraction
    fs::path temp_dir = fs::temp_directory_path() / "amk_test_asm";
    fs::remove_all(temp_dir); // Clean up any previous test runs

    // Extract the ASM package
    bool extraction_success = AddMusic::asm_package.extract(temp_dir);
    REQUIRE(extraction_success);

    // Verify the extraction directory was created
    REQUIRE(fs::exists(temp_dir));
    REQUIRE(fs::is_directory(temp_dir));

    // Check for expected ASM driver files
    REQUIRE(fs::exists(temp_dir / "main.asm"));
    REQUIRE(fs::exists(temp_dir / "Commands.asm"));
    REQUIRE(fs::exists(temp_dir / "CommandTable.asm"));

    // Verify the directory is not empty
    REQUIRE(!fs::is_empty(temp_dir));

    // === STEP 2: Compile the main.asm driver to memory using AsarBinding ===
    AsarBinding asar (temp_dir / "main.asm");
    bool compilation_success = asar.compileToBin();
    std::cerr << "== stderr from main.asm compilation ==" << std::endl << asar.getStderr() << std::endl;
    std::cerr << "== stdout from main.asm compilation ==" << std::endl << asar.getStdout() << std::endl;
    REQUIRE(compilation_success);

    // Clean up
    fs::remove_all(temp_dir);
}

TEST_CASE("Test songs package extraction", "[package][test_songs]")
{
    // Create temporary directory for extraction
    fs::path temp_songs_dir = fs::temp_directory_path() / "amk_test_songs";
    fs::remove_all(temp_songs_dir); // Clean up any previous test runs

    // Extract the test songs package
    bool extract_success = AddMusic::test_songs_package_01.extract(temp_songs_dir);
    REQUIRE(extract_success);

    // Verify the extraction directory was created
    REQUIRE(fs::exists(temp_songs_dir));
    REQUIRE(fs::is_directory(temp_songs_dir));

    // Clean up
    fs::remove_all(temp_songs_dir);
}

TEST_CASE("Using the boilerplate to patch a clean Super Mario World ROM", "[integration][rom][patching]")
{
    // Create temporary directories
    fs::path temp_boilerplate_dir = fs::temp_directory_path() / "amk_integration_boilerplate";
    fs::path temp_work_dir = fs::temp_directory_path() / "amk_integration_work";
    fs::path patched_rom_path = fs::temp_directory_path() / "patched_rom_boilerplate.smc";

    // Clean up any previous test runs
    fs::remove_all(temp_boilerplate_dir);
    fs::remove_all(temp_work_dir);
    fs::remove(patched_rom_path);

    // Step 1: Extract boilerplate package
    bool extract_success = AddMusic::boilerplate_package.extract(temp_boilerplate_dir);
    REQUIRE(extract_success);
    REQUIRE(fs::exists(temp_boilerplate_dir / "Addmusic_list.txt"));

    // Step 2: Use real SMW ROM if available, otherwise generate synthetic ROM
    fs::path smw_rom_path = getSmwRomPath();

    if (smw_rom_path.empty()) {
        SKIP("No SMW ROM found. If you want to proceed with this test, place a clean SMW ROM in the project "
            "directory with the name 'smw.smc', or set the SMW_ROM_LOCATION environment variable with the "
            "location of your SMW ROM.");
    }

    // Copy the SMW ROM to a temporary location to avoid modifying the original
    fs::path temp_rom_path = fs::temp_directory_path() / "amk_temp_smw.smc";
    if (fs::exists(temp_rom_path))
        fs::remove(temp_rom_path);
    fs::copy(smw_rom_path, temp_rom_path);
    INFO("Using real SMW ROM: " << fs::absolute(smw_rom_path).string());

    // Step 3: Patch the ROM using the extracted boilerplate
    try {
        AddMusic::ROMEnvironment rom_env(temp_rom_path, temp_boilerplate_dir);
        bool patch_success = rom_env.patchROM(patched_rom_path);
        REQUIRE(patch_success);
        REQUIRE(fs::exists(patched_rom_path));
        REQUIRE(fs::file_size(patched_rom_path) > fs::file_size(temp_rom_path)); // Patched ROM should be larger
    } catch (const std::exception& e) {
        FAIL("ROM patching failed with exception: " << e.what());
    }

    // Clean up
    fs::remove_all(temp_boilerplate_dir);
    fs::remove_all(temp_work_dir);
    fs::remove(temp_rom_path); // Safe to delete since we always copy/create in temp directory
    fs::remove(patched_rom_path);
}

void test_custom_songs_integration(Package& test_songs_pkg, std::string patched_rom_name = "patched_rom.smc")
{
    // Create temporary directories and populate them
    fs::path temp_boilerplate_dir = fs::temp_directory_path() / "amk_integration_boilerplate";
    fs::path temp_testsongs_dir = fs::temp_directory_path() / "amk_test_songs";
    fs::path patched_rom_path = fs::temp_directory_path() / patched_rom_name;

    bool extract_success = AddMusic::boilerplate_package.extract(temp_boilerplate_dir) &&
        test_songs_pkg.extract(temp_testsongs_dir);
    
    REQUIRE(extract_success);
    REQUIRE(fs::exists(temp_boilerplate_dir / "Addmusic_list.txt"));
    
    // Check the test song directory structure is what we expect.
    REQUIRE(fs::exists(temp_testsongs_dir / "music"));
    REQUIRE(fs::exists(temp_testsongs_dir / "samples"));
    REQUIRE(fs::exists(temp_testsongs_dir / "sample_groups"));

    // == STEP 1: Modify Addmusic_list.txt automatically. ==
    // Reads the Addmusic_list.txt.
    std::ifstream infile((temp_boilerplate_dir / "Addmusic_list.txt").string());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() ||
            (!line.empty() && (line[0] == ';' || line[0] == '#')) ||
            std::all_of(line.begin(), line.end(), ::isspace)
        ) continue;
        
        lines.push_back(line);
    }

    // Read the existent song files in the package.
    std::vector<fs::path> song_files;
    for (const auto& entry : fs::directory_iterator(temp_testsongs_dir / "music"))
    {
        fs::copy_file(entry.path(), temp_boilerplate_dir / "music" / entry.path().filename(), fs::copy_options::overwrite_existing);
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            song_files.push_back(entry.path());
    }

    // Replace lines in the existent Addmusic_list.txt.
    uint8_t song_index = 0x0A;
    for (const auto& song_file : song_files)
    {
        std::string song_index_str = hexDump(song_index++);
        std::string song_name = song_file.stem().string();
        std::string new_line = song_index_str + "  " + song_file.filename().string() + "\r";

        auto it = std::find_if(lines.begin(), lines.end(), [&song_index_str](const std::string& l_i) {
            // Needs the song index to be of 2 digits.
            return l_i.substr(0, 2) == song_index_str;
        });
        if (it != lines.end())
            *it = new_line; // Replace the line if the song name is already in the list.
        else
            lines.push_back(new_line); // Otherwise, add a new line.
    }

    // Rewrite the Addmusic_list.txt with the new content.
    std::ofstream outfile((temp_boilerplate_dir / "Addmusic_list.txt").string());
    for (const auto& l_i : lines)
        outfile << l_i << std::endl;
    outfile.close();

    // == STEP 2: Copy samplegroups ==
    // The content of every sample_groups file is copied into the Addmusic_sample groups.txt file.
    std::ofstream sample_groups_outfile((temp_boilerplate_dir / "Addmusic_sample groups.txt").string(), std::ios::app);
    for (const auto& entry : fs::directory_iterator(temp_testsongs_dir / "sample_groups"))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            std::ifstream sample_group_infile(entry.path().string());
            sample_groups_outfile << std::endl << sample_group_infile.rdbuf() << std::endl;
            sample_group_infile.close();
        }
    }

    // == STEP 3: Copy samples ==
    // The new sample files are copied into the boilerplate's samples folder, keeping the folder structure.
    for (const auto& entry : fs::recursive_directory_iterator(temp_testsongs_dir / "samples"))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".brr")
        {
            fs::path relative_path = fs::relative(entry.path(), temp_testsongs_dir / "samples");
            fs::copy_file(entry.path(), temp_boilerplate_dir / "samples" / relative_path, fs::copy_options::overwrite_existing);
        }
        else if (entry.is_directory())
        {
            fs::path relative_path = fs::relative(entry.path(), temp_testsongs_dir / "samples");
            fs::create_directories(temp_boilerplate_dir / "samples" / relative_path);
        }
    }

    // == STEP 4: Patch! ==
    fs::path smw_rom_path = getSmwRomPath();
    if (smw_rom_path.empty()) {
        SKIP("No SMW ROM found. If you want to proceed with this test, place a clean SMW ROM in the project "
            "directory with the name 'smw.smc', or set the SMW_ROM_LOCATION environment variable with the "
            "location of your SMW ROM.");
    }

    // Copy the SMW ROM to a temporary location to avoid modifying the original
    fs::path temp_rom_path = fs::temp_directory_path() / "amk_temp_smw.smc";
    if (fs::exists(temp_rom_path))
        fs::remove(temp_rom_path);
    fs::copy(smw_rom_path, temp_rom_path);
    INFO("Using real SMW ROM: " << fs::absolute(smw_rom_path).string());

    bool exception_thrown = false;
    std::string exception_string;

    try {
        AddMusic::ROMEnvironment rom_env(temp_rom_path, temp_boilerplate_dir);
        bool patch_success = rom_env.patchROM(patched_rom_path);
        REQUIRE(patch_success);
        REQUIRE(fs::exists(patched_rom_path));
        REQUIRE(fs::file_size(patched_rom_path) > fs::file_size(temp_rom_path)); // Patched ROM should be larger
        fs::copy(patched_rom_path, fs::current_path() / patched_rom_name, fs::copy_options::overwrite_existing); // Copy the patched ROM to the current directory for manual inspection if needed.
    } catch (const std::exception& e) {
        exception_thrown = true;
        exception_string = e.what();
    }
    
    // Clean up
    fs::remove(patched_rom_path);
    fs::remove_all(temp_boilerplate_dir);
    fs::remove_all(temp_testsongs_dir);

    // Conditional here to allow cleanup code to be ran before failing the test.
    if (exception_thrown) {
        FAIL("ROM patching failed with exception: " << exception_string);
    } else {
        SUCCEED(std::string() + "ROM patching successful! Go test it at: " + (fs::current_path() / patched_rom_name).string());
    }
}

TEST_CASE("Using songs from test_songs_01 and patch a ROM with it.", "[integration][rom][patching]")
{
    test_custom_songs_integration(AddMusic::test_songs_package_01, "patched_rom_test_songs_01.smc");
}

TEST_CASE("Using songs from test_songs_02 and patch a ROM with it.", "[integration][comments][rom][patching]")
{
    test_custom_songs_integration(AddMusic::test_songs_package_02, "patched_rom_test_songs_02.smc");
}

TEST_CASE("Using songs from test_songs_03 and patch a ROM with it.", "[integration][case-sensitiveness][rom][patching]")
{
    test_custom_songs_integration(AddMusic::test_songs_package_03, "patched_rom_test_songs_03.smc");
}
