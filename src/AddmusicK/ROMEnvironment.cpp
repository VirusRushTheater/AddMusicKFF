#include "ROMEnvironment.h"
#include "AddmusicLogging.h"
#include "Utility.h"
#include "asarBinding.h"

#include <AM405Remover.h>
#include <iostream>

using namespace AddMusic;

ROMEnvironment::ROMEnvironment(const fs::path& smw_rom, const fs::path& work_dir, EnvironmentOptions opts) :
	SPCEnvironment(work_dir, opts)
{
	// Translate the "bank optimizations" option into an address.
	bankStart = options.bankOptimizations ? 0x200000 : 0x080000;

	ROMName = smw_rom;
	readBinaryFile(ROMName, rom);

	_tryToCleanAM4Data();
	_tryToCleanAMMData();

	if (rom.size() % 0x8000 != 0)
	{
		romHeader.assign(rom.begin(), rom.begin() + 0x200);
		rom.erase(rom.begin(), rom.begin() + 0x200);
	}

	if (rom.size() <= 0x80000)
	{
		throw std::runtime_error("Your ROM is too small. Save a level in Lunar Magic or expand it with Lunar Expand, then try again.");
	}

	usingSA1 = (rom[SNESToPC(0xFFD5)] == 0x23 && options.allowSA1);

	_cleanROM();
}

bool ROMEnvironment::patchROM(const fs::path& patched_rom_location)
{
	bool visualizeSongs = false;

	justSPCsPlease = false;
	spc_build_plan = false;

	loadSampleList(work_dir / DEFAULT_SAMPLELIST_FILENAME);
	loadMusicList(work_dir / DEFAULT_SONGLIST_FILENAME);
	loadSFXList(work_dir / DEFAULT_SFXLIST_FILENAME);

	bool result = true;
	result &= _assembleSNESDriver();
	result &= _assembleSPCDriver();
	result &= _compileSFX();
	result &= _compileGlobalData();

	for (int i = 0; i < 256; i++)
	{
		// Load music file in memory.
		if (musics[i].exists)
			readTextFile(fs::absolute(musics[i].name), musics[i].text);
	}

	result &= _compileMusic();
	result &= _compileMusicROMSide();
	result &= _fixMusicPointers();

	result &= _generateSPCs();

	/*
	if (visualizeSongs)
		generatePNGs();
	*/

	result &= _assembleSNESDriverROMSide();

	if (result)
	{
		fs::path rom_folder = patched_rom_location.has_parent_path() ? patched_rom_location.parent_path() : ".";
		writeBinaryFile(patched_rom_location, patched_rom);
		generateMSC(rom_folder / (patched_rom_location.stem().string() + ".msc"));
	}

	return true;
}

bool ROMEnvironment::_cleanROM()
{
	_tryToCleanSampleToolData();

	if (rom[0x70000] == 0x3E && rom[0x70001] == 0x0E)	// If this is a "clean" ROM, then we don't need to do anything.
	{
		writeBinaryFile(driver_builddir / "SNES" / "temp.sfc", rom);
		return true;
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
				"your ROM, or your ROM is corrupted.";
			
			// There used to be an user prompt here.
			Logging::error(s.str());
			return false;
		}

		int romversion = *(unsigned char *)(rom.data() + SNESToPC(0x0E8004));

		if (romversion > DATA_VERSION)
		{
			std::stringstream s;		// // //
			s << "WARNING: This ROM was modified using a newer version of AddmusicK." << std::endl;
			s << "You can continue, but it is HIGHLY recommended that you upgrade AMK first." << std::endl;
			
			// There used to be another user prompt here.
			Logging::error(s.str());
			return false;
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

	// temp.sfc is now a clean ROM.
	writeBinaryFile(driver_builddir / "SNES" / "temp.sfc", rom);
	return true;
}

bool ROMEnvironment::_tryToCleanSampleToolData()
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

	if (found == false)
		return false;

	Logging::info("Sample Tool detected. Erasing data...");

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

	Logging::info(std::stringstream() << "Erased 0x" << hex6 << sizeOfErasedData << " bytes, of which 0x" << sampleDataSize << " were sample data.");
	return true;
}

bool ROMEnvironment::_tryToCleanAM4Data()
{
	if ((rom.size() % 0x8000 != 0 && rom[0x1940] == 0x22) || (rom.size() % 0x8000 == 0 && rom[0x1740] == 0x22))
	{
		if (rom.size() % 0x8000 == 0)
		{
			Logging::error("Addmusic 4.05 ROMs can only be cleaned if they have a header. This does not apply to any other aspect of the program.");
			return false;
		}
		char **am405argv;
		std::string romname_str = ROMName.string();

		am405argv = (char **)malloc(sizeof(char **) * 2);
		am405argv[1] = (char *)malloc(romname_str.size() + 1);
		strcpy(am405argv[1], romname_str.c_str());
		am405argv[1][romname_str.size()] = 0;
		std::cout << "Attempting to erase data from Addmusic 4.05:" << std::endl;
		removeAM405Data(2, am405argv);

		readBinaryFile(romname_str, rom);					// Reopen the file.
		if (rom[0x255] == 0x5C)
		{
			int moreASMData = ((rom[0x255+3] << 16) | (rom[0x255+2] << 8) | (rom[0x255+1])) - 8;
			clearRATS(SNESToPC(moreASMData));
		}
		int romiSPCProgramAddress = (unsigned char)rom[0x2E9] | ((unsigned char)rom[0x2EE]<<8) | ((unsigned char)rom[0x2F3]<<16);
		clearRATS(SNESToPC(romiSPCProgramAddress) - 12 + 0x200);
	}

	return true;
}

bool ROMEnvironment::_tryToCleanAMMData()
{
	if ((rom.size() % 0x8000 != 0 && findRATS(0x078200)) || (rom.size() % 0x8000 == 0 && findRATS(0x078000)))		// Since RATS tags only need to be in banks 0x10+, a tag here signals AMM usage.
	{
		if (rom.size() % 0x8000 == 0)
		{
			Logging::error("AddmusicM ROMs can only be cleaned if they have a header. This does not\napply to any other aspect of the program.");
			return false;
		}

		if (!fs::exists("INIT.asm"))
			Logging::error("AddmusicM was detected.  In order to remove it from this ROM, you must put AddmusicM's INIT.asm as well as xkasAnti and a clean ROM (named clean.smc) in\nthe same folder as this program. Then attempt to run this program once more.");

		std::cout << "AddmusicM detected.  Attempting to remove..." << std::endl;
		system( (((std::string)("perl addmusicMRemover.pl ")) + ROMName.string()).c_str());
		system( (((std::string)("xkasAnti clean.smc ")) + ROMName.string() + "INIT.asm").c_str());
	}

	return true;
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

int ROMEnvironment::findFreeSpace(unsigned int size, int start, std::vector<uint8_t> &ROM)
{
	if (size == 0)
		Logging::error("Internal error: Requested free ROM space cannot be 0 bytes.");
	if (size > 0x7FF8)
		Logging::error("Internal error: Requested free ROM space cannot exceed 0x7FF8 bytes.");

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

bool ROMEnvironment::_assembleSNESDriverROMSide()
{
	Logging::debug("Generating SNES driver...");

	std::string patch;

	readTextFile(driver_builddir / "SNES" / "patch.asm", patch);

	replaceHexValue((uint16_t)reuploadPos, "!ExpARAMRet = ", patch);
	replaceHexValue((uint16_t)mainLoopPos, "!DefARAMRet = ", patch);
	replaceHexValue((uint8_t)songCount, "!SongCount = ", patch);

	int pos;

	pos = patch.find("MusicPtrs:");
	if (pos == -1)
	{
		Logging::error("Error: \"MusicPtrs:"" could not be found.");		// // //
		return false;
	}

	patch = patch.substr(0, pos);

	{
		std::string patch2;
		readTextFile(driver_builddir / "SNES" / "patch2.asm", patch2);
		patch += patch2;
	}

	std::stringstream musicPtrStr; musicPtrStr << "MusicPtrs: \ndl ";
	std::stringstream samplePtrStr; samplePtrStr << "\n\nSamplePtrs:\ndl ";
	std::stringstream sampleLoopPtrStr; sampleLoopPtrStr << "\n\nSampleLoopPtrs:\ndw ";
	std::stringstream musicIncbins; musicIncbins << "\n\n";
	std::stringstream sampleIncbins; sampleIncbins << "\n\n";

	Logging::debug("Writing music files...");

	for (int i = 0; i < songCount; i++)
	{
		if (musics[i].exists == true && i > highestGlobalSong)
		{
			int requestSize;
			int freeSpace;
			fs::path musicBinPath {driver_builddir / "SNES" / "bin" / ("music" + hex<2>(i) + ".bin")};
			requestSize = fs::file_size(musicBinPath);
			freeSpace = findFreeSpace(requestSize, bankStart, rom);
			if (freeSpace == -1)
			{
				Logging::error("Your ROM is out of free space.");
				return false;
			}

			freeSpace = PCToSNES(freeSpace);
			musicPtrStr << "music" << hex2 << i << "+8";
			musicIncbins << "org $" << hex6 << freeSpace << "\nmusic" << hex2 << i << ": incbin \"bin/music" + hex<2>(i) + ".bin\"" << std::endl;
		}
		else
		{
			musicPtrStr << "$" << hex6 << 0;

		}

		if ((i & 0xF) == 0xF && i != songCount-1)
			musicPtrStr << "\ndl ";
		else if (i != songCount-1)
			musicPtrStr << ", ";
	}

	Logging::debug("Writing sample files...");

	for (int i = 0; i < samples.size(); i++)
	{
		if (samples[i].exists)
		{
			std::vector<uint8_t> temp;
			temp.resize(samples[i].data.size() + 10);
			temp[0] = 'S';
			temp[1] = 'T';
			temp[2] = 'A';
			temp[3] = 'R';

			temp[4] = (samples[i].data.size()+2-1) & 0xFF;
			temp[5] = (samples[i].data.size()+2-1) >> 8;

			temp[6] = (~(samples[i].data.size()+2-1) & 0xFF);
			temp[7] = (~(samples[i].data.size()+2-1) >> 8);

			temp[8] = samples[i].data.size() & 0xFF;
			temp[9] = samples[i].data.size() >> 8;

			for (unsigned int j = 0; j < samples[i].data.size(); j++)
				temp[j+10] = samples[i].data[j];
			fs::path filename {driver_builddir / "SNES" / "bin" / ("brr" + hex<2>(i) + ".bin")};
			writeBinaryFile(filename, temp);

			int requestSize = fs::file_size(filename);
			int freeSpace = findFreeSpace(requestSize, bankStart, rom);
			if (freeSpace == -1)
			{
				Logging::error("Error: Your ROM is out of free space.");
				return false;
			}

			freeSpace = PCToSNES(freeSpace);
			samplePtrStr << "brr" << hex2 << i << "+8";
			sampleIncbins << "org $" << hex6 << freeSpace << "\nbrr" << hex2 << i << ": incbin \"bin/brr" << hex2 << i << ".bin\"" << std::endl;

		}
		else
			samplePtrStr << "$" << hex6 << 0;

		sampleLoopPtrStr << "$" << hex4 << samples[i].loopPoint;


		if ((i & 0xF) == 0xF && i != samples.size()-1)
		{
			samplePtrStr << "\ndl ";
			sampleLoopPtrStr << "\ndw ";
		}
		else if (i != samples.size()-1)
		{
			samplePtrStr << ", ";
			sampleLoopPtrStr << ", ";
		}
	}

	patch += "pullpc\n\n";

	musicPtrStr << "\ndl $FFFFFF\n";
	samplePtrStr << "\ndl $FFFFFF\n";

	patch += musicPtrStr.str();
	patch += samplePtrStr.str();
	patch += sampleLoopPtrStr.str();

	//patch += "";

	patch += musicIncbins.str();
	patch += sampleIncbins.str();

	replaceHexValue((uint8_t)highestGlobalSong, "!GlobalMusicCount = #", patch);

	std::stringstream ss;
	ss << "\n\norg !SPCProgramLocation" << "\nincbin \"bin/main.bin\"";
	patch += ss.str();

	std::string undoPatch;
	readTextFile(driver_builddir / "SNES" / "AMUndo.asm", undoPatch);
	patch.insert(patch.begin(), undoPatch.begin(), undoPatch.end());

	writeTextFile(driver_builddir / "SNES" / "temppatch.asm", patch);

	Logging::debug("Final compilation...");

	AsarBinding asar4 (driver_builddir / "SNES" / "temppatch.asm");
	if (!asar4.patchToRom(driver_builddir / "SNES" / "temp.sfc", true))
	{
		asar4.printErrors();
		Logging::error("asar reported an error while patching the ROM.");
		return false;
	}

	patched_rom.clear();
	readBinaryFile(driver_builddir / "SNES" / "temp.sfc", patched_rom);
	patched_rom.insert(patched_rom.begin(), romHeader.begin(), romHeader.end());

	return true;
}

void ROMEnvironment::generateMSC(const fs::path& location)
{
	std::stringstream text;

	for (int i = 0; i < 256; i++)
	{
		if (musics[i].exists)
		{
			text << hex<2>(i) << "\t" << 0 << "\t" << musics[i].title << "\n";
			text << hex<2>(i) << "\t" << 1 << "\t" << musics[i].title << "\n";
			//fprintf(fout, "%2X\t0\t%s\n", i, musics[i].title.c_str());
			//fprintf(fout, "%2X\t1\t%s\n", i, musics[i].title.c_str());
		}
	}
	writeTextFile(location, text.str());
}

bool ROMEnvironment::_compileMusicROMSide()
{
	// Used to be part of AddmusicK.cpp:compileMusic()
	std::stringstream songSampleList;
	std::string s;

	songSampleListSize = 8;

	songSampleList << "db $53, $54, $41, $52\t\t\t\t; Needed to stop Asar from treating this like an xkas patch.\n";
	songSampleList << "dw SGEnd-SampleGroupPtrs-$01\ndw SGEnd-SampleGroupPtrs-$01^$FFFF\nSampleGroupPtrs:\n\n";

	for (int i = 0; i < songCount; i++)
	{
		if (i % 16 == 0)
			songSampleList << "\ndw ";
		if (musics[i].exists == false)
			songSampleList << "$" << hex4 << 0;
		else
			songSampleList << "SGPointer" << hex2 << i;
		songSampleListSize += 2;

		if (i != songCount - 1 && (i & 0xF) != 0xF)
			songSampleList << ", ";
		//s = songSampleList.str();
	}

	songSampleList << "\n\n";

	for (int i = 0; i < songCount; i++)
	{
		if (!musics[i].exists) continue;

		songSampleListSize++;

		songSampleList << "\n" << "SGPointer" << hex2 << i << ":\n";

		if (i > highestGlobalSong)
		{
			songSampleList << "db $" << hex2 << musics[i].mySamples.size() << "\ndw";
			for (unsigned int j = 0; j < musics[i].mySamples.size(); j++)
			{
				songSampleListSize+=2;
				songSampleList << " $" << hex4 << (int)(musics[i].mySamples[j]);
				if (j != musics[i].mySamples.size() - 1)
					songSampleList << ",";
			}
		}

	}
	songSampleList << "\nSGEnd:";
	s = songSampleList.str();
	std::stringstream tempstream;

	tempstream << "org $" << hex6 << PCToSNES(findFreeSpace(songSampleListSize, bankStart, rom)) << "\n\n" << std::endl;

	s.insert(0, tempstream.str());

	writeTextFile(driver_builddir / "SNES" / "SongSampleList.asm", s);
	return true;
}
