#pragma once

#include <vector>
#include <cstdint>
#include <filesystem>

#include "SPCEnvironment.h"

namespace fs = std::filesystem;

namespace AddMusic
{

/**
 * @brief Stuff pertaining to the ROM.
 */
class ROMEnvironment
{
public:
	bool loadROM(fs::path _rom_path);
	void prepareROM();
	void tryToCleanSampleToolData();
	void tryToCleanAM4Data();
	void tryToCleanAMMData();
	void cleanROM();

	/**
	 * @brief Returns a position in the ROM with the specified amount of free
	 * space, starting at the specified position. NOT using SNES addresses!
	 * 
	 * This function writes a RATS address at the position returned. 
	 */
	int findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM);

	int clearRATS(int PCaddr);
	bool findRATS(int addr);

private:
	// Attributes
	SPCEnvironment* spcenv;
	fs::path rom_path;

	std::vector<uint8_t> rom;
	std::vector<uint8_t> romHeader;

	bool usingSA1;	// Does the ROM use SA1?
};

}