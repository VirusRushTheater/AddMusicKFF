#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * SMWTestROMGenerator - Generates minimal Super Mario World compatible ROMs for testing
 *
 * This class creates synthetic ROM files that contain the necessary structures
 * for AddmusicK testing without using copyrighted Nintendo content.
 *
 * Dependencies: None (uses only standard C++ libraries)
 */
class SMWTestROMGenerator {
public:
    // ROM size options (in bytes)
    enum class ROMSize {
        SIZE_512KB = 512 * 1024,    // 0.5 MB
        SIZE_1MB = 1024 * 1024,     // 1 MB
        SIZE_2MB = 2 * 1024 * 1024, // 2 MB
        SIZE_4MB = 4 * 1024 * 1024  // 4 MB
    };

    // ROM type options
    enum class ROMType {
        LOROM,  // Most common for SMW
        HIROM
    };

    /**
     * Configuration for ROM generation
     */
    struct Config {
        ROMSize size = ROMSize::SIZE_1MB;
        ROMType type = ROMType::LOROM;
        std::string gameTitle = "TESTROM";
        std::string gameCode = "TEST";
        uint8_t version = 0;
        bool fastROM = false;
        bool expansionRAM = false;

        // AddmusicK specific settings
        bool includeMusicSpace = true;  // Reserve space for music data
        bool includeSampleSpace = true; // Reserve space for samples
    };

    static const Config defaultConfig;

    /**
     * Generate a test ROM with the specified configuration
     */
    static std::vector<uint8_t> generateROM(const Config& config = defaultConfig);

    /**
     * Save generated ROM to file
     */
    static bool saveROM(const fs::path& filepath, const std::vector<uint8_t>& romData);

    /**
     * Generate and save ROM in one step
     */
    static bool generateAndSaveROM(const fs::path& filepath, const Config& config = defaultConfig);

    /**
     * Validate that a ROM has basic SMW-compatible structure
     */
    static bool validateROM(const std::vector<uint8_t>& romData);

private:
    // SMC header constants
    static constexpr uint32_t SMC_HEADER_SIZE = 512;
    static constexpr uint8_t SMC_ID_BYTE = 0xAA;

    // SNES header offsets (relative to ROM start, after SMC header)
    static constexpr uint32_t SNES_HEADER_OFFSET = 0x7FC0;
    static constexpr uint32_t SNES_HEADER_SIZE = 64;

    // AddmusicK relevant memory areas
    static constexpr uint32_t SEQ_OFS_ADDRESS = 0xFE000;  // Music sequence pointers
    static constexpr uint32_t PCMSET_ADD_ADDRESS = 0xFCF00; // Sample set pointers
    static constexpr uint32_t PCMDATA_ADDRESS = 0xFD000;  // Sample data area

    // Helper methods
    static void writeSMCHeader(std::vector<uint8_t>& rom, const Config& config);
    static void writeSNESHeader(std::vector<uint8_t>& rom, const Config& config);
    static void initializeROMData(std::vector<uint8_t>& rom, const Config& config);
    static void setupAddmusicKSpace(std::vector<uint8_t>& rom, const Config& config);
    static void calculateChecksums(std::vector<uint8_t>& rom);

    // Address conversion utilities
    static uint32_t pcToSnes(uint32_t pcAddr, ROMType type);
    static uint32_t snesToPc(uint32_t snesAddr, ROMType type);

    // Checksum calculation
    static uint16_t calculateSNESChecksum(const std::vector<uint8_t>& data);
    static uint16_t calculateSNESComplement(uint16_t checksum);
};