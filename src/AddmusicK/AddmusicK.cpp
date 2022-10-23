
#include "AM405Remover.h"

#include "AddmusicK.hpp"
#include "Utility.h"

using namespace AddMusic;

Addmusic_error AddMusicK::loadRom(std::string rom_path)
{
	ROMName = rom_path;
	std::string tempROMName = ROMName.cStr();

	// ROM is loaded into memory (this->rom)
	if (fileExists(tempROMName + ".smc") && fileExists(tempROMName + ".sfc"))
		AMKERROR("Error: Ambiguity detected; there were two ROMs with the specified name (one\nwith a .smc extension and one with a .sfc extension). Either delete one or\ninclude the extension in your filename.", ADDMUSICERROR_IO_ERROR);
	
	else if (fileExists(tempROMName + ".smc"))
		tempROMName += ".smc";
	else if (fileExists(tempROMName + ".sfc"))
		tempROMName += ".sfc";
	else if (fileExists(tempROMName)) ;
	else
		AMKERROR("ROM not found.", ADDMUSICERROR_IO_ERROR);
	
	ROMName = tempROMName;
	openFile(ROMName, rom);

	this->tryToCleanAM4Data();
	this->tryToCleanAMMData();

	// Clear any previous iteration of AddMusic's info, if there is any.
	tryToCleanAM4Data();
	tryToCleanAMMData();

	// Transfers the first 512 bytes of the SMC file into the header variable.
	if (rom.size() % 0x8000 != 0)
	{
		romHeader.assign(rom.begin(), rom.begin() + 0x200);
		rom.erase(rom.begin(), rom.begin() + 0x200);
	}

	if (rom.size() <= 0x80000)
		AMKERROR("Error: Your ROM is too small. Save a level in Lunar Magic or expand it with\nLunar Expand, then try again.", ADDMUSICERROR_ROM_TOO_SMALL);

	usingSA1 = (rom[SNESToPC(0xFFD5)] == 0x23 && p.allowSA1);

	this->cleanROM();

	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::tryToCleanAM4Data()
{
	AMKREQUIRESROM

	if ((rom.size() % 0x8000 != 0 && rom[0x1940] == 0x22) || (rom.size() % 0x8000 == 0 && rom[0x1740] == 0x22))
	{
		if (rom.size() % 0x8000 == 0)
			AMKERROR("Addmusic 4.05 ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.", ADDMUSICERROR_NO_AM4_HEADER);

		char **am405argv;
		am405argv = 	(char **)malloc(sizeof(char **) * 2);
		am405argv[1] = 	(char *)malloc(ROMName.size() + 1);
		strcpy(am405argv[1], ROMName.cStr());
		am405argv[1][ROMName.size()] = 0;

		AMKLOG("Attempting to erase data from Addmusic 4.05");

		removeAM405Data(2, am405argv);
		openFile(ROMName, rom);					// Reopen the file.
		if (rom[0x255] == 0x5C)
		{
			int moreASMData = ((rom[0x255+3] << 16) | (rom[0x255+2] << 8) | (rom[0x255+1])) - 8;
			clearRATS(SNESToPC(moreASMData));
		}
		int romiSPCProgramAddress = (unsigned char)rom[0x2E9] | ((unsigned char)rom[0x2EE]<<8) | ((unsigned char)rom[0x2F3]<<16);
		clearRATS(SNESToPC(romiSPCProgramAddress) - 12 + 0x200);
	}

	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::tryToCleanAMMData()
{
	AMKREQUIRESROM

	if ((rom.size() % 0x8000 != 0 && findRATS(0x078200)) || (rom.size() % 0x8000 == 0 && findRATS(0x078000)))		// Since RATS tags only need to be in banks 0x10+, a tag here signals AMM usage.
	{
		if (rom.size() % 0x8000 == 0)
			printError("AddmusicM ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.", true);

		if (fileExists("INIT.asm") == false)
			printError("AddmusicM was detected.  In order to remove it from this ROM, you must put\nAddmusicM's INIT.asm as well as xkasAnti and a clean ROM (named clean.smc) in\nthe same folder as this program. Then attempt to run this program once more.", true);

		AMKLOG("AddmusicM detected.  Attempting to remove...");

		// TODO: Merge these utilities into the program
		execute( ((std::string)("perl addmusicMRemover.pl ")) + (std::string)ROMName);
		execute( ((std::string)("xkasAnti clean.smc ")) + (std::string)ROMName + "INIT.asm");
	}

	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::cleanROM()
{
	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::tryToCleanSampleToolData()
{
	return ADDMUSICERROR_SUCCESS;
}