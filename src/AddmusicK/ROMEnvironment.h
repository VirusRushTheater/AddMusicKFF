#pragma once

#include <vector>
#include <cstdint>
#include <filesystem>

#include "SPCEnvironment.h"

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief Working environment that includes Super Mario World ROM hacking
 * capabilities.
 * If you only want to generate playable SPCs you will need just an instance
 * of SPCEnvironment, which won't require a ROM to initialize.
 */
class ROMEnvironment : public SPCEnvironment
{
public:
	ROMEnvironment(const fs::path& smw_rom, const fs::path& work_dir, EnvironmentOptions opts = EnvironmentOptions());

	bool patchROM(const fs::path& patched_rom_location);

	bool _cleanROM();
	bool _tryToCleanSampleToolData();
	bool _tryToCleanAM4Data();
	bool _tryToCleanAMMData();

	/**
	 * @brief Returns a position in the ROM with the specified amount of free
	 * space, starting at the specified position. NOT using SNES addresses!
	 * 
	 * This function writes a RATS address at the position returned. 
	 */
	int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM);

	int clearRATS(int PCaddr);
	bool findRATS(int addr);

	bool _assembleSNESDriverROMSide();
	bool _compileMusicROMSide();
	void generateMSC(const fs::path& location);

protected:
	// Attributes
	fs::path ROMName;

	std::vector<uint8_t> rom;
	std::vector<uint8_t> romHeader;

	std::vector<uint8_t> patched_rom;

	uint24_t bankStart {0x200000};
};

}