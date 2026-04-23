/**
 * SMWTestROMGenerator Demo
 *
 * This program demonstrates how to use the SMWTestROMGenerator
 * to create synthetic test ROMs for AddmusicK testing.
 */

#include "SMWTestROMGenerator.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::cout << "SMWTestROMGenerator Demo" << std::endl;
    std::cout << "========================" << std::endl;

    // Create output directory
    fs::path outputDir = "test_roms";
    if (!fs::exists(outputDir)) {
        fs::create_directory(outputDir);
    }

    // Generate different types of test ROMs
    std::vector<SMWTestROMGenerator::Config> configs = {
        // Default 1MB LoROM
        {
            SMWTestROMGenerator::ROMSize::SIZE_1MB,
            SMWTestROMGenerator::ROMType::LOROM,
            "TESTROM1",
            "TEST",
            0, false, false, true, true
        },
        // 2MB LoROM with FastROM
        {
            SMWTestROMGenerator::ROMSize::SIZE_2MB,
            SMWTestROMGenerator::ROMType::LOROM,
            "FASTROM2",
            "FAST",
            1, true, false, true, true
        },
        // 512KB HiROM
        {
            SMWTestROMGenerator::ROMSize::SIZE_512KB,
            SMWTestROMGenerator::ROMType::HIROM,
            "HIROM512",
            "HIRO",
            0, false, false, true, true
        },
        // Minimal ROM without AddmusicK space
        {
            SMWTestROMGenerator::ROMSize::SIZE_1MB,
            SMWTestROMGenerator::ROMType::LOROM,
            "MINIMAL1",
            "MINI",
            0, false, false, false, false
        }
    };

    for (size_t i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        std::string filename = "test_rom_" + std::to_string(i + 1) + ".smc";
        fs::path filepath = outputDir / filename;

        std::cout << "Generating ROM " << (i + 1) << ": " << filename << std::endl;
        std::cout << "  Size: " << (static_cast<uint32_t>(config.size) / 1024 / 1024) << "MB" << std::endl;
        std::cout << "  Type: " << (config.type == SMWTestROMGenerator::ROMType::LOROM ? "LoROM" : "HiROM") << std::endl;
        std::cout << "  Title: " << config.gameTitle << std::endl;
        std::cout << "  FastROM: " << (config.fastROM ? "Yes" : "No") << std::endl;
        std::cout << "  Music Space: " << (config.includeMusicSpace ? "Yes" : "No") << std::endl;
        std::cout << "  Sample Space: " << (config.includeSampleSpace ? "Yes" : "No") << std::endl;

        bool success = SMWTestROMGenerator::generateAndSaveROM(filepath, config);
        if (success) {
            // Validate the generated ROM
            auto romData = SMWTestROMGenerator::generateROM(config);
            bool valid = SMWTestROMGenerator::validateROM(romData);

            std::cout << "  Status: " << (valid ? "Valid" : "Invalid") << std::endl;
            std::cout << "  File size: " << fs::file_size(filepath) << " bytes" << std::endl;
        } else {
            std::cout << "  Status: Failed to save" << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "Demo complete! Check the 'test_roms' directory for generated ROMs." << std::endl;
    std::cout << "These ROMs can be used for AddmusicK testing without copyright concerns." << std::endl;

    return 0;
}