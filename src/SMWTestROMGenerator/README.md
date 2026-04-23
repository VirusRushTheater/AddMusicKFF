# SMWTestROMGenerator

A C++ library for generating synthetic Super Mario World compatible ROMs for testing purposes. This allows you to create test ROMs without using copyrighted Nintendo content.

## Purpose

AddmusicK and similar ROM hacking tools need to work with actual SMW ROM files for testing. However, distributing copyrighted ROMs with open source projects is illegal. This generator creates minimal but valid SMW-compatible ROMs that can be used for testing AddmusicK functionality.

## Dependencies

**None!** This library uses only standard C++ features:

- C++17 standard library
- `<vector>`, `<string>`, `<filesystem>`, `<fstream>`, `<algorithm>`, `<cstring>`

No external libraries or dependencies required.

## Features

- **Multiple ROM sizes**: 512KB, 1MB, 2MB, 4MB
- **ROM types**: LoROM (default) and HiROM
- **Configurable options**: FastROM, expansion RAM, custom titles
- **AddmusicK compatibility**: Pre-allocated space for music and sample data
- **Valid checksums**: Proper SNES header with calculated checksums
- **SMC format**: Includes copier header for compatibility

## Usage

### Basic Usage

```cpp
#include "SMWTestROMGenerator.h"

int main() {
    // Generate a default 1MB test ROM
    SMWTestROMGenerator::Config config;
    auto romData = SMWTestROMGenerator::generateROM(config);

    // Save to file
    SMWTestROMGenerator::saveROM("test.smc", romData);

    // Or generate and save in one step
    SMWTestROMGenerator::generateAndSaveROM("test.smc", config);

    return 0;
}
```

### Advanced Configuration

```cpp
SMWTestROMGenerator::Config config;
config.size = SMWTestROMGenerator::ROMSize::SIZE_2MB;
config.type = SMWTestROMGenerator::ROMType::LOROM;
config.gameTitle = "MYTESTROM";
config.gameCode = "TEST";
config.version = 1;
config.fastROM = true;
config.expansionRAM = false;
config.includeMusicSpace = true;   // Reserve space for AddmusicK music data
config.includeSampleSpace = true;  // Reserve space for AddmusicK samples

auto romData = SMWTestROMGenerator::generateROM(config);
```

### Validation

```cpp
auto romData = SMWTestROMGenerator::generateROM();
bool isValid = SMWTestROMGenerator::validateROM(romData);
```

## Building the Demo

```bash
cd src/SMWTestROMGenerator
mkdir build
cd build
cmake ..
make
./SMWTestROMGeneratorDemo
```

This will generate several test ROMs in the `test_roms/` directory.

## ROM Structure

Generated ROMs include:

1. **SMC Header** (512 bytes): Copier compatibility
2. **SNES Internal Header** (64 bytes at 0x7FC0): Game information, checksums
3. **ROM Data**: Filled with NOP instructions (0xEA)
4. **Reset Routine**: Simple infinite loop at 0x8000
5. **AddmusicK Space** (optional):
   - Music sequence table at 0xFE000
   - Sample set table at 0xFCF00
   - Sample data area at 0xFD000

## Technical Details

### Address Mapping
- **LoROM**: Standard mapping used by most SMW hacks
- **HiROM**: Alternative mapping for larger ROMs

### Checksums
- Calculated using proper SNES algorithm
- Both checksum and complement stored in header

### File Format
- **.smc extension**: Standard for SNES ROMs with copier header
- Total size = SMC header (512) + ROM data size

## Integration with Tests

Example test integration:

```cpp
TEST_CASE("ROM Environment with synthetic ROM", "[rom][environment]") {
    // Generate test ROM
    SMWTestROMGenerator::Config config;
    config.size = SMWTestROMGenerator::ROMSize::SIZE_1MB;
    config.includeMusicSpace = true;
    config.includeSampleSpace = true;

    auto romData = SMWTestROMGenerator::generateROM(config);
    fs::path testRomPath = "test_minimal.smc";
    SMWTestROMGenerator::saveROM(testRomPath, romData);

    // Use in test
    ROMEnvironment rom_env(testRomPath, list_folder, options);
    // ... test operations ...

    fs::remove(testRomPath); // Clean up
}
```

## Legal Note

These generated ROMs contain no copyrighted material and are safe to use in testing. They are not playable games but provide the necessary structure for testing ROM manipulation tools like AddmusicK.