
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

	// == At this point we already have a clean SMW ROM at this->rom.

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
			AMKERROR("AddmusicM ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.", ADDMUSICERROR_NO_AMM_HEADER);

		if (fileExists("INIT.asm") == false)
			AMKERROR("AddmusicM was detected.  In order to remove it from this ROM, you must put\nAddmusicM's INIT.asm as well as xkasAnti and a clean ROM (named clean.smc) in\nthe same folder as this program. Then attempt to run this program once more.", ADDMUSICERROR_NO_INIT_ASM);

		AMKLOG("AddmusicM detected.  Attempting to remove...");

		// TODO: Merge these utilities into the program
		execute( ((std::string)("perl addmusicMRemover.pl ")) + (std::string)ROMName);
		execute( ((std::string)("xkasAnti clean.smc ")) + (std::string)ROMName + "INIT.asm");
	}

	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::cleanROM()
{
	AMKREQUIRESROM

	tryToCleanSampleToolData();

	if (rom[0x70000] == 0x3E && rom[0x70001] == 0x0E)	// If this is a "clean" ROM, then we don't need to do anything.
	{
		// writeFile("asm/SNES/temp.sfc", rom);
		return ADDMUSICERROR_SUCCESS;
	}
	else
	{
		//"New Super Mario World Sample Utility 2.0 by smkdan"

		std::string romprogramname;
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8000));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8001));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8002));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8003));

		// No @AMK signature, even if there should be, for an edited ROM that
		// reaches this point.
		if (romprogramname != "@AMK")
		{
			std::stringstream s;
			s << "Error: The identifier for this ROM, \"" << romprogramname << "\", could not be identified. It should\n"
				"be \"@AMK\". This either means that some other program has modified this area of\n"
				"your ROM, or your ROM is corrupted.";
			
			// The CLI is lenient about letting you continue even after this problem
			// arises. The library approach just returns an error code.
			AMKERROR(s, ADDMUSICERROR_NO_AMK_SIGNATURE);

			// if (forceNoContinuePrompt) {
			// 	std::cerr << s.str() << std::endl;
			// 	quit(1);
			// }
			// std::cout << s.str() << " Continue? (Y or N)" << std::endl;

			// int c = '\0';
			// while (c != 'y' && c != 'Y' && c != 'n' && c != 'N')
			// 	c = getchar();

			// if (c == 'n' || c == 'N')
			// 	quit(1);
		}

		int romversion = *(unsigned char *)(rom.data() + SNESToPC(0x0E8004));

		if (romversion > DATA_VERSION)
		{
			std::stringstream s;
			s << "WARNING: This ROM was modified using a newer version of AddmusicK." << std::endl;
			s << "You can continue, but it is HIGHLY recommended that you upgrade AMK first." << std::endl;

			// The library approach lets you continue, even if you had this problem.
			AMKWARNING(s);

			// if (forceNoContinuePrompt) {
			// 	std::cerr << s.str() << std::endl;
			// 	quit(1);
			// }
			// std::cout << s.str() << "Continue anyway? (Y or N)" << std::endl;

			// int c = '\0';
			// while (c != 'y' && c != 'Y' && c != 'n' && c != 'N')
			// 	c = getchar();

			// if (c == 'n' || c == 'N')
			// 	quit(1);
		}

		int address = SNESToPC(*(unsigned int *)(rom.data() + 0x70005) & 0xFFFFFF);	// Address, in this case, is the sample numbers list.
		clearRATS(address - 8);								// So erase it all.

		int baseAddress = SNESToPC(*(unsigned int *)(rom.data() + 0x70008));		// Address is now the address of the pointers to the songs and samples.
		bool erasingSamples = false;

		int i = 0;
		while (true)
		{
			address = *(unsigned int *)(rom.data() + baseAddress + i*3) & 0xFFFFFF;
			if (address == 0xFFFFFF)						// 0xFFFFFF indicates an end of pointers.
			{
				if (erasingSamples == false)
					erasingSamples = true;
				else
					break;
			}
			else
			{
				if (address != 0)
					clearRATS(SNESToPC(address - 8));
			}

			i++;
		}
	}

	// writeFile("asm/SNES/temp.sfc", rom);

	return ADDMUSICERROR_SUCCESS;
}

Addmusic_error AddMusicK::tryToCleanSampleToolData()
{
	unsigned int i;
	bool found = false;

	// TODO: Implement some algorithm that can make this search routine more efficient.
	for (i = 0; i < rom.size() - 50; i++)
	{
		// Scans within the ROM for this string.
		if (strncmp((char *)rom.data() + i, "New Super Mario World Sample Utility 2.0 by smkdan", 50) == 0)
		{
			found = true;
			break;
		}
	}

	if (found == false)
	{
		AMKLOG("No Sample tool info detected.");
		return ADDMUSICERROR_SUCCESS;
	}

	AMKLOG("Sample Tool detected.  Erasing data...");

	int hackPos = i - 8;

	i += 0x36;

	int sizeOfErasedData = 0;

	bool removed[0x100];
	for (int j = 0; j < 0x100; j++) removed[j] = false;

	for (int j = 0; j < 0x207; j++)
	{
		if (removed[rom[j+i]]) continue;
		sizeOfErasedData += clearRATS(SNESToPC(rom[j+i]* 0x10000 + 0x8000));
		removed[rom[j+i]] = true;
	}

	int sampleDataSize = sizeOfErasedData;

	sizeOfErasedData += clearRATS(hackPos);

	std::stringstream s;
	s << "Erased 0x" << hex6 << sizeOfErasedData << " bytes, of which 0x" << sampleDataSize << " were sample data.";
	AMKLOG(s);

	return ADDMUSICERROR_SUCCESS;
}

int AddMusicK::clearRATS(int offset)
{
	int size = ((rom[offset + 5] << 8) | rom[offset + 4]) + 8;
	int r = size;
	while (size >= 0)
		rom[offset + size--] = 0;

	return r+1;
}

int AddMusicK::PCToSNES(int addr)
{
	if (addr < 0 || addr >= 0x400000)
		return -1;

	addr = ((addr << 1) & 0x7F0000) | (addr & 0x7FFF) | 0x8000;

	if (!usingSA1 && (addr & 0xF00000) == 0x700000)
		addr |= 0x800000;

	if (usingSA1 && addr >= 0x400000)
		addr += 0x400000;
	return addr;
}

int AddMusicK::SNESToPC(int addr)
{
	if (addr < 0 || addr > 0xFFFFFF ||		// not 24bit
		(addr & 0xFE0000) == 0x7E0000 ||	// wram
		(addr & 0x408000) == 0x000000)		// hardware regs
		return -1;
	if (usingSA1 && addr >= 0x808000)
		addr -= 0x400000;
	addr = ((addr & 0x7F0000) >> 1 | (addr & 0x7FFF));
	return addr;
}