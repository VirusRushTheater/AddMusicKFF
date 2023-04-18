#include "ROMEnvironment.h"
#include "AddmusicLogging.h"
#include "Utility.h"

#include <iostream>

using namespace AddMusic;

void ROMEnvironment::prepareROM()
{
	tryToCleanAM4Data();
	tryToCleanAMMData();

	if (rom.size() % 0x8000 != 0)
	{
		romHeader.assign(rom.begin(), rom.begin() + 0x200);
		rom.erase(rom.begin(), rom.begin() + 0x200);
	}
	//rom.openFromFile(ROMName);

	if (rom.size() <= 0x80000)
		throw AddmusicException("Error: Your ROM is too small. Save a level in Lunar Magic or expand it with\nLunar Expand, then try again.");

	usingSA1 = (rom[SNESToPC(0xFFD5)] == 0x23 && options.allowSA1);

	cleanROM();
}

void ROMEnvironment::cleanROM()
{
	tryToCleanSampleToolData();

	if (rom[0x70000] == 0x3E && rom[0x70001] == 0x0E)	// If this is a "clean" ROM, then we don't need to do anything.
	{
		writeFile("asm/SNES/temp.sfc", rom);
		return;
	}
	else
	{
		//"New Super Mario World Sample Utility 2.0 by smkdan"

		std::string romprogramname;
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8000));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8001));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8002));
		romprogramname += (char)*(rom.data() + SNESToPC(0x0E8003));

		if (romprogramname != "@AMK")
		{
			std::stringstream s;
			s << "Error: The identifier for this ROM, \"" << romprogramname << "\", could not be identified. It should\n"
				"be \"@AMK\". This either means that some other program has modified this area of\n"
				"your ROM, or your ROM is corrupted.";		// // //
			if (forceNoContinuePrompt) {
				std::cerr << s.str() << std::endl;
				quit(1);
			}
			std::cout << s.str() << " Continue? (Y or N)" << std::endl;


			int c = '\0';
			while (c != 'y' && c != 'Y' && c != 'n' && c != 'N')
				c = getchar();

			if (c == 'n' || c == 'N')
				quit(1);

		}

		int romversion = *(unsigned char *)(rom.data() + SNESToPC(0x0E8004));

		if (romversion > DATA_VERSION)
		{
			std::stringstream s;		// // //
			s << "WARNING: This ROM was modified using a newer version of AddmusicK." << std::endl;
			s << "You can continue, but it is HIGHLY recommended that you upgrade AMK first." << std::endl;
			
			if (forceNoContinuePrompt) {
				std::cerr << s.str() << std::endl;
				quit(1);
			}
			std::cout << s.str() << "Continue anyway? (Y or N)" << std::endl;

			int c = '\0';
			while (c != 'y' && c != 'Y' && c != 'n' && c != 'N')
				c = getchar();

			if (c == 'n' || c == 'N')
				quit(1);
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

	writeFile("asm/SNES/temp.sfc", rom);
}

void ROMEnvironment::tryToCleanSampleToolData()
{
	unsigned int i;
	bool found = false;

	for (i = 0; i < rom.size() - 50; i++)
	{

		if (strncmp((char *)rom.data() + i, "New Super Mario World Sample Utility 2.0 by smkdan", 50) == 0)
		{
			found = true;
			break;
		}
	}

	if (found == false) return;

	std::cout << "Sample Tool detected.  Erasing data..." << std::endl;

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

	std::cout << "Erased 0x" << hex6 << sizeOfErasedData << " bytes, of which 0x" << sampleDataSize << " were sample data.";
}

void ROMEnvironment::tryToCleanAM4Data()
{
	if ((rom.size() % 0x8000 != 0 && rom[0x1940] == 0x22) || (rom.size() % 0x8000 == 0 && rom[0x1740] == 0x22))
	{
		if (rom.size() % 0x8000 == 0)
			printError("Addmusic 4.05 ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.", true);

		char **am405argv;
		am405argv = (char **)malloc(sizeof(char **) * 2);
		am405argv[1] = (char *)malloc(ROMName.size() + 1);
		strcpy(am405argv[1], ROMName.cStr());
		am405argv[1][ROMName.size()] = 0;
		std::cout << "Attempting to erase data from Addmusic 4.05:" << std::endl;
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
}

void ROMEnvironment::tryToCleanAMMData()
{
	if ((rom.size() % 0x8000 != 0 && findRATS(0x078200)) || (rom.size() % 0x8000 == 0 && findRATS(0x078000)))		// Since RATS tags only need to be in banks 0x10+, a tag here signals AMM usage.
	{
		if (rom.size() % 0x8000 == 0)
			printError("AddmusicM ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.", true);

		if (fileExists("INIT.asm") == false)
			printError("AddmusicM was detected.  In order to remove it from this ROM, you must put\nAddmusicM's INIT.asm as well as xkasAnti and a clean ROM (named clean.smc) in\nthe same folder as this program. Then attempt to run this program once more.", true);

		std::cout << "AddmusicM detected.  Attempting to remove..." << std::endl;
		execute( ((std::string)("perl addmusicMRemover.pl ")) + (std::string)ROMName);
		execute( ((std::string)("xkasAnti clean.smc ")) + (std::string)ROMName + "INIT.asm");
	}
}

int ROMEnvironment::findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM)
{
	if (size == 0)
		throw AddmusicException("Internal error: Requested free ROM space cannot be 0 bytes.");
	if (size > 0x7FF8)
		throw AddmusicException("Internal error: Requested free ROM space cannot exceed 0x7FF8 bytes.");

	size_t pos = 0;
	size_t runningSpace = 0;
	size += 8;
	int i;

	for (i = start; (unsigned)i < ROM.size(); i++)
	{
		if (runningSpace == size)
		{
			pos = i;
			break;
		}

		if (i % 0x8000 == 0)
			runningSpace = 0;

		if ((unsigned)i < ROM.size() - 4 && memcmp(&ROM[i], "STAR", 4) == 0)
		{
			unsigned short RATSSize = ROM[i+4] | ROM[i+5] << 8;
			unsigned short sizeInv = (ROM[i+6] | ROM[i+7] << 8) ^ 0xFFFF;
			if (RATSSize != sizeInv)
			{
				runningSpace += 1;
				continue;
			}
			i += RATSSize + 8;	// Would be nine if the loop didn't auto increment.
			runningSpace = 0;

		}
		else if (ROM[i] == 0 || options.aggressive)
		{
			runningSpace += 1;
		}
		else
		{
			runningSpace = 0;
		}
	}

	if (runningSpace == size)
		pos = i;

	if (pos == 0)
	{
		if (start == 0x080000)
			return -1;
		else
			return findFreeSpace(size, 0x080000, rom);
	}

	pos -= size;

	ROM[pos+0] = 'S';
	ROM[pos+1] = 'T';
	ROM[pos+2] = 'A';
	ROM[pos+3] = 'R';
	pos += 4;
	size -= 9;			// Not -8.  -8 would accidentally protect one too many bytes.
	ROM[pos+0] = size & 0xFF;
	ROM[pos+1] = size >> 8;
	size ^= 0xFFFF;
	ROM[pos+2] = size & 0xFF;
	ROM[pos+3] = size >> 8;
	pos -= 4;
	return pos;
}

int ROMEnvironment::SNESToPC(int addr)					// Thanks to alcaro.
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

int ROMEnvironment::PCToSNES(int addr)
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

bool ROMEnvironment::findRATS(int offset)
{
	if (rom[offset] != 0x53) {
		return false;
	}
	if (rom[offset+1] != 0x54) {
		return false;
	}
	if (rom[offset+2] != 0x41) {
		return false;
	}
	if (rom[offset+3] != 0x52) {
		return false;
	}
	return true;
}

int ROMEnvironment::clearRATS(int offset)
{
	int size = ((rom[offset + 5] << 8) | rom[offset+4]) + 8;
	int r = size;
	while (size >= 0)
		rom[offset + size--] = 0;
	return r+1;
}
