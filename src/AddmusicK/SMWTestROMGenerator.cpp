#include "SMWTestROMGenerator.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>

const SMWTestROMGenerator::Config SMWTestROMGenerator::defaultConfig;

std::vector<uint8_t> SMWTestROMGenerator::generateROM(const Config& config) {
    // Calculate total ROM size (ROM data only, no SMC header for simplicity)
    uint32_t totalSize = static_cast<uint32_t>(config.size);

    std::vector<uint8_t> rom(totalSize, 0x00);

    writeSNESHeader(rom, config);
    initializeROMData(rom, config);
    setupAddmusicKSpace(rom, config);
    calculateChecksums(rom);

    return rom;
}

bool SMWTestROMGenerator::saveROM(const fs::path& filepath, const std::vector<uint8_t>& romData) {
    try {
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            return false;
        }
        file.write(reinterpret_cast<const char*>(romData.data()), romData.size());
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

bool SMWTestROMGenerator::generateAndSaveROM(const fs::path& filepath, const Config& config) {
    auto romData = generateROM(config);
    return saveROM(filepath, romData);
}

bool SMWTestROMGenerator::validateROM(const std::vector<uint8_t>& romData) {
    if (romData.size() < SMC_HEADER_SIZE + SNES_HEADER_SIZE) {
        return false;
    }

    // Check SMC header
    if (romData[8] != SMC_ID_BYTE) {
        return false;
    }

    // Check SNES header magic bytes
    uint32_t snesHeaderStart = SMC_HEADER_SIZE + SNES_HEADER_OFFSET;
    if (romData[snesHeaderStart + 0x1C] != 0xAA || romData[snesHeaderStart + 0x1D] != 0xBB) {
        return false;
    }

    return true;
}

void SMWTestROMGenerator::writeSMCHeader(std::vector<uint8_t>& rom, const Config& config) {
    // SMC header is at the beginning of the file
    // Byte 8: ID byte (0xAA)
    rom[8] = SMC_ID_BYTE;

    // Bytes 9-10: ROM type and size info
    uint16_t romType = 0;
    if (config.type == ROMType::HIROM) {
        romType |= 0x01;  // HiROM bit
    }
    if (config.fastROM) {
        romType |= 0x10;  // FastROM bit
    }

    // Calculate ROM size code (number of 128KB banks)
    uint8_t sizeCode = 0;
    uint32_t romSizeKB = static_cast<uint32_t>(config.size) / 1024;
    while (romSizeKB > 128) {
        romSizeKB /= 2;
        sizeCode++;
    }

    rom[9] = sizeCode;
    rom[10] = static_cast<uint8_t>(romType);
}

void SMWTestROMGenerator::writeSNESHeader(std::vector<uint8_t>& rom, const Config& config) {
    uint32_t headerStart = SNES_HEADER_OFFSET;

    // Clear header area
    std::fill(rom.begin() + headerStart, rom.begin() + headerStart + SNES_HEADER_SIZE, 0x00);

    // Game title (21 bytes, padded with spaces)
    std::string title = config.gameTitle.substr(0, 21);
    title.resize(21, ' ');
    std::memcpy(&rom[headerStart], title.c_str(), 21);

    // ROM makeup / type
    uint8_t makeup = 0;
    if (config.type == ROMType::HIROM) {
        makeup |= 0x01;
    }
    if (config.fastROM) {
        makeup |= 0x10;
    }
    rom[headerStart + 0x15] = makeup;

    // ROM type / size
    uint8_t romSize = 0;
    uint32_t sizeKB = static_cast<uint32_t>(config.size) / 1024;
    while (sizeKB > 128) {
        sizeKB /= 2;
        romSize++;
    }
    rom[headerStart + 0x16] = romSize;

    // SRAM size
    rom[headerStart + 0x17] = 0x00;  // No SRAM

    // Country code
    rom[headerStart + 0x18] = 0x00;  // Japan

    // License code
    rom[headerStart + 0x19] = 0x00;

    // Version number
    rom[headerStart + 0x1A] = config.version;

    // Checksum complement (will be calculated later)
    rom[headerStart + 0x1C] = 0xAA;
    rom[headerStart + 0x1D] = 0xBB;  // Magic bytes

    // Checksum (will be calculated later)
    rom[headerStart + 0x1E] = 0x00;
    rom[headerStart + 0x1F] = 0x00;

    // Native vectors (point to valid reset code)
    uint32_t resetVector = pcToSnes(0x8000, config.type);

    // Native mode vectors
    rom[headerStart + 0x3C] = resetVector & 0xFF;        // RESET
    rom[headerStart + 0x3D] = (resetVector >> 8) & 0xFF;
    rom[headerStart + 0x3E] = 0x00;  // NMI
    rom[headerStart + 0x3F] = 0x80;

    // Emulation mode vectors
    rom[headerStart + 0x3C] = resetVector & 0xFF;        // RESET
    rom[headerStart + 0x3D] = (resetVector >> 8) & 0xFF;
}

void SMWTestROMGenerator::initializeROMData(std::vector<uint8_t>& rom, const Config& config) {
    uint32_t romDataStart = 0;
    uint32_t romDataSize = static_cast<uint32_t>(config.size);

    // Fill ROM with NOP instructions (0xEA) for basic functionality
    std::fill(rom.begin() + romDataStart, rom.begin() + romDataStart + romDataSize, 0xEA);

    // Add a simple reset routine at 0x8000
    uint32_t resetAddr = romDataStart + 0x8000;
    if (resetAddr + 16 < rom.size()) {
        // Simple infinite loop (for testing purposes)
        rom[resetAddr] = 0xEA;     // NOP
        rom[resetAddr + 1] = 0xEA; // NOP
        rom[resetAddr + 2] = 0xEA; // NOP
        rom[resetAddr + 3] = 0xEA; // NOP
        rom[resetAddr + 4] = 0x80; // BRA -4 (infinite loop)
        rom[resetAddr + 5] = 0xFA;
    }
}

void SMWTestROMGenerator::setupAddmusicKSpace(std::vector<uint8_t>& rom, const Config& config) {
    uint32_t romDataStart = 0;

    // Write the AddmusicK identifier "@AMK" and version at the expected location
    // For LoROM, SNESToPC(0x0E8000) = 0x070000
    uint32_t amkIdentifierAddr = romDataStart + 0x070000;
    if (amkIdentifierAddr + 5 < rom.size()) {
        rom[amkIdentifierAddr] = '@';
        rom[amkIdentifierAddr + 1] = 'A';
        rom[amkIdentifierAddr + 2] = 'M';
        rom[amkIdentifierAddr + 3] = 'K';
        rom[amkIdentifierAddr + 4] = config.version;  // Version byte
    }

    if (!config.includeMusicSpace && !config.includeSampleSpace) {
        return;
    }

    // Initialize music sequence pointer table (SEQ_OFS)
    if (config.includeMusicSpace && SEQ_OFS_ADDRESS < static_cast<uint32_t>(config.size)) {
        uint32_t seqTableAddr = romDataStart + SEQ_OFS_ADDRESS;
        // Clear the table (256 entries of 3 bytes each = 768 bytes)
        std::fill(rom.begin() + seqTableAddr, rom.begin() + seqTableAddr + 768, 0x00);
    }

    // Initialize sample set pointer table (PCMSET_ADD)
    if (config.includeSampleSpace && PCMSET_ADD_ADDRESS < static_cast<uint32_t>(config.size)) {
        uint32_t pcmTableAddr = romDataStart + PCMSET_ADD_ADDRESS;
        // Clear the table (256 entries of 3 bytes each = 768 bytes)
        std::fill(rom.begin() + pcmTableAddr, rom.begin() + pcmTableAddr + 768, 0x00);
    }

    // Reserve space for sample data (PCMDATA)
    if (config.includeSampleSpace && PCMDATA_ADDRESS < static_cast<uint32_t>(config.size)) {
        uint32_t pcmDataAddr = romDataStart + PCMDATA_ADDRESS;
        uint32_t pcmDataSize = 0x1000;  // 4KB for sample data
        if (pcmDataAddr + pcmDataSize < rom.size()) {
            std::fill(rom.begin() + pcmDataAddr, rom.begin() + pcmDataAddr + pcmDataSize, 0x00);
        }
    }
}

void SMWTestROMGenerator::calculateChecksums(std::vector<uint8_t>& rom) {
    uint32_t romDataStart = 0;
    uint32_t romDataSize = rom.size();

    // Calculate SNES checksum
    uint16_t checksum = calculateSNESChecksum(std::vector<uint8_t>(
        rom.begin() + romDataStart,
        rom.begin() + romDataStart + romDataSize
    ));

    uint16_t complement = calculateSNESComplement(checksum);

    // Write checksums to SNES header
    uint32_t headerStart = SNES_HEADER_OFFSET;
    rom[headerStart + 0x1E] = checksum & 0xFF;
    rom[headerStart + 0x1F] = (checksum >> 8) & 0xFF;
    rom[headerStart + 0x1C] = complement & 0xFF;
    rom[headerStart + 0x1D] = (complement >> 8) & 0xFF;
}

uint16_t SMWTestROMGenerator::calculateSNESChecksum(const std::vector<uint8_t>& data) {
    uint16_t sum = 0;
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 < data.size()) {
            uint16_t word = data[i] | (data[i + 1] << 8);
            sum = (sum + word) & 0xFFFF;
        } else {
            sum = (sum + data[i]) & 0xFFFF;
        }
    }
    return sum;
}

uint16_t SMWTestROMGenerator::calculateSNESComplement(uint16_t checksum) {
    return (~checksum) & 0xFFFF;
}

uint32_t SMWTestROMGenerator::pcToSnes(uint32_t pcAddr, ROMType type) {
    if (type == ROMType::LOROM) {
        // LoROM mapping: PC = ((SNES & 0x7FFF) | ((SNES & 0x7F8000) >> 1))
        uint32_t bank = (pcAddr / 0x8000) * 2;
        uint32_t offset = pcAddr % 0x8000;
        return (bank << 16) | offset | 0x8000;
    } else {
        // HiROM mapping: PC = SNES & 0x3FFFFF
        return pcAddr | 0xC00000;
    }
}

uint32_t SMWTestROMGenerator::snesToPc(uint32_t snesAddr, ROMType type) {
    if (type == ROMType::LOROM) {
        // LoROM mapping: SNES = ((PC & 0x7FFF) | ((PC & 0x7F8000) << 1)) | 0x8000
        uint32_t bank = (snesAddr >> 16) / 2;
        uint32_t offset = snesAddr & 0x7FFF;
        return (bank * 0x8000) + offset;
    } else {
        // HiROM mapping: SNES = PC | 0xC00000
        return snesAddr & 0x3FFFFF;
    }
}