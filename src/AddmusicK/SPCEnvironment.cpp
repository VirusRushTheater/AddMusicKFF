#include <cstring>
#include <algorithm>

#include "AddmusicException.h"
#include "asarBinding.h"
#include "SPCEnvironment.h"
#include "Utility.h"

#include <iostream>

using namespace AddMusic;

// Will be evaluated relatively to work_dir to find source codes.
constexpr const char DEFAULT_BUILD_FOLDER[] {"build"};

// Default file names
constexpr const char DEFAULT_SONGLIST_FILENAME[] {"Addmusic_list.txt"};
constexpr const char DEFAULT_SAMPLELIST_FILENAME[] {"Addmusic_sample groups.txt"};
constexpr const char DEFAULT_SFXLIST_FILENAME[] {"Addmusic_sound effects.txt"};

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& src_dir) :
	work_dir(work_dir),
	src_dir(src_dir),	
	build_dir(src_dir / DEFAULT_BUILD_FOLDER)
{
	if (!fs::exists(work_dir))
		throw fs::filesystem_error("The directory with music has not been found", work_dir, std::error_code());
	if (!fs::exists(src_dir))
		throw fs::filesystem_error("SPC driver source directory not found", src_dir, std::error_code());
	
	// Clean and create an empty "build" directory ([src/build])
	if (fs::exists(build_dir))
		deleteDir(build_dir);
	fs::create_directory(build_dir);
}

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, const fs::path& src_dir, const SPCEnvironmentOptions& opts) :
	SPCEnvironment(rom_path, work_dir)
{
	options = opts;
}

void SPCEnvironment::loadMusicList()
{
	std::string musicFile;
	readTextFile(work_dir / DEFAULT_SONGLIST_FILENAME, musicFile);

	if (musicFile[musicFile.length()-1] != '\n')
		musicFile += '\n';

	unsigned int i = 0;

	bool inGlobals = false;
	bool inLocals = false;
	bool gettingName = false;
	int index = -1;
	int shallowSongCount = 0;

	std::string tempName;

	while (i < musicFile.length())
	{
		if (isspace(musicFile[i]) && !gettingName)
		{
			i++;
			continue;
		}

		if (strncmp(musicFile.c_str() + i, "Globals:", 8) == 0)
		{
			inGlobals = true;
			inLocals = false;
			i+=8;
			continue;
		}

		if (strncmp(musicFile.c_str() + i, "Locals:", 7) == 0)
		{
			inGlobals = false;
			inLocals = true;
			i+=7;
			continue;
		}

		if (!inGlobals && !inLocals)
			throw AddmusicException("Error: Could not find \"Globals:\" label in list.txt");

		if (index < 0)
		{
			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index = musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index = musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index = musicFile[i++] - 'a' + 10;
			else throw AddmusicException("Invalid number in list.txt.");

			index <<= 4;


			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index |= musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index |= musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index |= musicFile[i++] - 'a' + 10;
			else if (isspace(musicFile[i])) index >>= 4;
			else throw AddmusicException("Invalid number in list.txt.");

			if (!isspace(musicFile[i]))
				throw AddmusicException("Invalid number in list.txt.");
			if (inGlobals)
				highestGlobalSong = std::max(highestGlobalSong, index);
			if (inLocals)
				if (index <= highestGlobalSong)
					throw AddmusicException("Error: Local song numbers must be greater than the largest global song number.");
		}
		else
		{
			if (musicFile[i] == '\n' || musicFile[i] == '\r')
			{
				musics[index].name = tempName;
				if (inLocals && options.justSPCsPlease == false)
				{
					readTextFile(work_dir / "music" / tempName, musics[index].text);
				}
				musics[index].exists = true;
				index = -1;
				i++;
				shallowSongCount++;
				gettingName = false;
				tempName.clear();
				continue;

			}
			gettingName = true;
			tempName += musicFile[i++];
		}
	}

	if (verbose)
		printf("Read in all %d songs.\n", shallowSongCount);

	for (int i = 255; i >= 0; i--)
	{
		if (musics[i].exists)
		{
			songCount = i+1;
			break;
		}
	}
}

void SPCEnvironment::loadSampleList()
{
	std::string str;
	readTextFile(work_dir / DEFAULT_SAMPLELIST_FILENAME, str);

	std::string groupName;
	std::string tempName;

	unsigned int i = 0;
	bool gettingGroupName = false;
	bool gettingSampleName = false;


	while (i < str.length())
	{
		if (gettingGroupName == false && gettingSampleName == false)
		{
			if (groupName.length() == 0)
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '#')
				{
					i++;
					gettingGroupName = true;
					groupName.clear();
					continue;
				}
			}
			else
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '{')
				{
					i++;
					gettingSampleName = true;
					continue;
				}
				else
				{
					throw AddmusicException("Error parsing sample groups.txt.  Expected opening curly brace.", true);
				}
			}
		}
		else if (gettingGroupName == true)
		{
			if (isspace(str[i]))
			{
				std::unique_ptr<BankDefine> sg = std::make_unique<BankDefine>();
				sg->name = groupName;
				bankDefines.push_back(std::move(sg));
				i++;
				gettingGroupName = false;
				continue;
			}
			else
			{
				groupName += str[i];
				i++;
				continue;
			}
		}
		else if (gettingSampleName)
		{
			if (tempName.length() > 0)
			{
				if (str[i] == '\"')
				{
					tempName.erase(tempName.begin(), tempName.begin() + 1);
					bankDefines[bankDefines.size() - 1]->samples.push_back(std::make_unique<std::string>(tempName));
					bankDefines[bankDefines.size() - 1]->importants.push_back(false);
					tempName.clear();
					i++;
					continue;
				}
				else
				{
					tempName += str[i];
					i++;
				}
			}
			else
			{
				if (isspace(str[i]))
				{
					i++;
					continue;
				}
				else if (str[i] == '\"')
				{
						i++;
						tempName += ' ';
						continue;
				}
				else if (str[i] == '}')
				{
					gettingSampleName = false;
					gettingGroupName = false;
					groupName.clear();
					i++;
					continue;
				}
				else if (str[i] == '!')
				{
					if (bankDefines[bankDefines.size() - 1]->importants.size() == 0)
						throw AddmusicException("Error parsing Addmusic_sample groups.txt: Importance specifier ('!') must come after asample declaration, not before it.", true);
					bankDefines[bankDefines.size() - 1]->importants[bankDefines[bankDefines.size() - 1]->importants.size() - 1] = true;
					i++;
				}
				else
				{
					throw AddmusicException("Error parsing sample groups.txt.  Expected opening quote.", true);
				}
			}
		}
	}
}

void SPCEnvironment::loadSFXList()
{
	std::string str;
	readTextFile(work_dir / DEFAULT_SFXLIST_FILENAME, str);

	if (str[str.length()-1] != '\n')
		str += '\n';

	unsigned int i = 0;

	bool in1DF9 = false;
	bool in1DFC = false;
	bool gettingName = false;
	int index = -1;
	bool isPointer = false;
	bool doNotAdd0 = false;

	std::string tempName;

	int SFXCount = 0;

	while (i < str.length())
	{
		if (isspace(str[i]) && !gettingName)
		{
			i++;
			continue;
		}

		if (strncmp(str.c_str() + i, "SFX1DF9:", 8) == 0)
		{
			in1DF9 = true;
			in1DFC = false;
			i+=8;
			continue;
		}

		if (strncmp(str.c_str() + i, "SFX1DFC:", 8) == 0)
		{
			in1DF9 = false;
			in1DFC = true;
			i+=8;
			continue;
		}

		if (!in1DF9 && !in1DFC)
			throw AddmusicException("Error: Could not find \"SFX1DF9:\" label in sound effects.txt", true);

		if (index < 0)
		{
			if      ('0' <= str[i] && str[i] <= '9') index = str[i++] - '0';
			else if ('A' <= str[i] && str[i] <= 'F') index = str[i++] - 'A' + 10;
			else if ('a' <= str[i] && str[i] <= 'f') index = str[i++] - 'a' + 10;
			else
				throw AddmusicException("Invalid number in sound effects.txt.", true);

			index <<= 4;


			if      ('0' <= str[i] && str[i] <= '9') index |= str[i++] - '0';
			else if ('A' <= str[i] && str[i] <= 'F') index |= str[i++] - 'A' + 10;
			else if ('a' <= str[i] && str[i] <= 'f') index |= str[i++] - 'a' + 10;
			else if (isspace(str[i])) index >>= 4;
			else
				throw AddmusicException("Invalid number in sound effects.txt.", true);


			if (!isspace(str[i]))
				throw AddmusicException("Invalid number in sound effects.txt.", true);
		}
		else
		{
			if (str[i] == '*' && tempName.length() == 0)
			{
				isPointer = true;
				i++;
			}
			else if (str[i] == '?' && tempName.length() == 0)
			{
				doNotAdd0 = true;
				i++;
			}
			else if (str[i] == '\n' || str[i] == '\r')
			{

					if (in1DF9)
					{
						if (!isPointer)
							soundEffectsDF9[index].name = tempName;
						else
							soundEffectsDF9[index].pointName = tempName;

						soundEffectsDF9[index].exists = true;

						if (doNotAdd0)
							soundEffectsDF9[index].add0 = false;
						else
							soundEffectsDF9[index].add0 = true;

						if (!isPointer)
							readTextFile(work_dir / "1DF9" / tempName, soundEffectsDF9[index].text);
					}
					else
					{
						if (!isPointer)
							soundEffectsDFC[index].name = tempName;
						else
							soundEffectsDFC[index].pointName = tempName;

						soundEffectsDFC[index].exists = true;

						if (doNotAdd0)
							soundEffectsDFC[index].add0 = false;
						else
							soundEffectsDFC[index].add0 = true;

						if (!isPointer)
							readTextFile(work_dir / "1DFC" / tempName, soundEffectsDFC[index].text);
					}

					index = -1;
					i++;
					SFXCount++;
					gettingName = false;
					tempName.clear();
					isPointer = false;
					doNotAdd0 = false;
					continue;

			}
			else
			{
				gettingName = true;
				tempName += str[i++];

			}
		}
	}
}

int SPCEnvironment::assembleSNESDriver()
{
	int programUploadPos_tmp; 
	std::string patch;
	readTextFile(build_dir / "SNES" / "patch.asm", patch);
	programUploadPos_tmp = scanInt(patch, "!DefARAMRet = ");

	programUploadPos = programUploadPos_tmp;

	return programUploadPos_tmp;
}

void SPCEnvironment::assembleSPCDriver()
{
	std::string patch;
	readTextFile(build_dir / "main.asm", patch);
	programPos = scanInt(patch, "base ");
	// if (verbose)
	// 	std::cout << "Compiling main SPC program, pass 1." << std::endl;

	AsarBinding asarpatch(build_dir / "main.asm");
	asarpatch.compileToBin();
	if (asarpatch.hasErrors())
		throw AddmusicException(std::string("asar reported the following errors while assembling asm/main.asm:\n\n") + asarpatch.getStderr(), true);

	std::string temptxt;
	readTextFile("temp.txt", temptxt);
	mainLoopPos = scanInt(temptxt, "MainLoopPos: ");
	reuploadPos = scanInt(temptxt, "ReuploadPos: ");
	noSFX = (temptxt.find("NoSFX is enabled") != -1);
	if (sfxDump && noSFX) {
		throw AddmusicException("The sound driver build does not support sound effects due to the !noSFX flag\r\nbeing enabled in asm/UserDefines.asm, yet you requested to dump SFX. There will\r\nbe no new SPC dumps of the sound effects since the data is not included by\r\ndefault, nor is the playback code for the sound effects.", false);
		sfxDump = false;
	}

	programSize = asarpatch.getProgramSize();
}

void SPCEnvironment::compileSFX()
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 1; j < 256; j++)
		{
			soundEffects[i][j].bank = i;
			soundEffects[i][j].index = j;
			if (soundEffects[i][j].pointName.length() > 0)
			{
				for (int k = 1; k < 256; k++)
				{
					if (soundEffects[i][j].pointName == soundEffects[i][k].name)
					{
						soundEffects[i][j].pointsTo = k;
						break;
					}
				}
				if (soundEffects[i][j].pointsTo == 0)
				{
					std::ostringstream r;
					r << std::hex << j;
					throw AddmusicException(std::string("Error: The sound effect that sound effect 0x") + r.str() + std::string(" points to could not be found."), true);
				}
			}
		}
	}
}

void SPCEnvironment::compileGlobalData()
{
	int DF9DataTotal = 0;
	int DFCDataTotal = 0;
	int DF9Count = 0;
	int DFCCount = 0;

	std::ostringstream oss;

	std::vector<unsigned short> DF9Pointers, DFCPointers;

	for (int i = 255; i >= 0; i--)
	{
		if (soundEffects[0][i].exists)
		{
			DF9Count = i;
			break;
		}
	}

	for (int i = 255; i >= 0; i--)
	{
		if (soundEffects[1][i].exists)
		{
			DFCCount = i;
			break;
		}
	}

	for (int i = 0; i <= DF9Count; i++)
	{
		if (soundEffects[0][i].exists && soundEffects[0][i].pointsTo == 0)
		{
			soundEffects[0][i].posInARAM = DFCCount * 2 + DF9Count * 2 + programPos + programSize + DF9DataTotal;
			soundEffects[0][i].compile();	// TODO: Watch this functon for errorCount.
			DF9Pointers.push_back(DF9DataTotal + (DF9Count + DFCCount) * 2 + programSize + programPos);
			DF9DataTotal += soundEffects[0][i].data.size() + soundEffects[0][i].code.size();
		}
		else if (soundEffects[0][i].exists == false)
		{
			DF9Pointers.push_back(0xFFFF);
		}
		else
		{
			if (i > soundEffects[0][i].pointsTo)
				DF9Pointers.push_back(DF9Pointers[soundEffects[0][i].pointsTo]);
			else
				throw AddmusicException("Error: A sound effect that is a pointer to another sound effect must come after\nthe sound effect that it points to.", true);
		}
	}

	// if (errorCount > 0)
	// 	throw AddmusicException("There were errors when compiling the sound effects.  Compilation aborted.  Your\nROM has not been modified.", true);

	for (int i = 0; i <= DFCCount; i++)
	{
		if (soundEffects[1][i].exists && soundEffects[1][i].pointsTo == 0)
		{
			soundEffects[1][i].posInARAM = DFCCount * 2 + DF9Count * 2 + programPos + programSize + DF9DataTotal + DFCDataTotal;
			soundEffects[1][i].compile();	// TODO: Watch this functon for errorCount.
			DFCPointers.push_back(DFCDataTotal + DF9DataTotal + (DF9Count + DFCCount) * 2 + programSize + programPos);
			DFCDataTotal += soundEffects[1][i].data.size() + soundEffects[1][i].code.size();
		}
		else if (soundEffects[1][i].exists == false)
		{
			DFCPointers.push_back(0xFFFF);
		}
		else
		{
			if (i > soundEffects[1][i].pointsTo)
				DFCPointers.push_back(DFCPointers[soundEffects[1][i].pointsTo]);
			else
				throw AddmusicException("Error: A sound effect that is a pointer to another sound effect must come after\nthe sound effect that it points to.", true);
		}
	}

	// if (errorCount > 0)
	// 	throw AddmusicException("There were errors when compiling the sound effects.  Compilation aborted.  Your\nROM has not been modified.", true);

	if (verbose)
	{
		std::cout << "Total space used by 1DF9 sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DF9DataTotal + DF9Count * 2) << std::dec << std::endl;
		std::cout << "Total space used by 1DFC sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DFCDataTotal + DFCCount * 2) << std::dec << std::endl;
	}

	std::cout << "Total space used by all sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DF9DataTotal + DF9Count * 2 + DFCDataTotal + DFCCount * 2) << std::dec << std::endl;

	DF9Pointers.erase(DF9Pointers.begin(), DF9Pointers.begin() + 1);
	DFCPointers.erase(DFCPointers.begin(), DFCPointers.begin() + 1);

	writeBinaryFile(build_dir / "SFX1DF9Table.bin", DF9Pointers);
	writeBinaryFile(build_dir / "SFX1DFCTable.bin", DFCPointers);

	std::vector<uint8_t> allSFXData;

	for (int i = 0; i <= DF9Count; i++)
	{
		for (unsigned int j = 0; j < soundEffects[0][i].data.size(); j++)
			allSFXData.push_back(soundEffects[0][i].data[j]);
		for (unsigned int j = 0; j < soundEffects[0][i].code.size(); j++)
			allSFXData.push_back(soundEffects[0][i].code[j]);
	}

	for (int i = 0; i <= DFCCount; i++)
	{
		for (unsigned int j = 0; j < soundEffects[1][i].data.size(); j++)
			allSFXData.push_back(soundEffects[1][i].data[j]);
		for (unsigned int j = 0; j < soundEffects[1][i].code.size(); j++)
			allSFXData.push_back(soundEffects[1][i].code[j]);
	}

	writeBinaryFile(build_dir / "SFXData.bin", allSFXData);

	std::string str;
	readTextFile(build_dir / "main.asm", str);

	int pos;

	pos = str.find("SFXTable0:");
	if (pos == -1)
		throw AddmusicException("Error: SFXTable0 not found in main.asm.", true);
	str.insert(pos+10, "\r\nincbin \"SFX1DF9Table.bin\"\r\n");

	pos = str.find("SFXTable1:");
	if (pos == -1)
		throw AddmusicException("Error: SFXTable1 not found in main.asm.", true);
	str.insert(pos+10, "\r\nincbin \"SFX1DFCTable.bin\"\r\nincbin \"SFXData.bin\"\r\n");

	writeTextFile(build_dir / "tempmain.asm", str);

	if (verbose)
		std::cout << "Compiling main SPC program, pass 2." << std::endl;

	AsarBinding main_with_sfx {build_dir / "tempmain.asm"};
	main_with_sfx.compileToFile(build_dir / "main.bin");
	if (main_with_sfx.hasErrors())
		throw AddmusicException(std::string("asar reported the following errors while assembling asm/main.asm:\n\n") + main_with_sfx.getStderr(), true);
	
	programSize = fs::file_size(build_dir / "main.bin");

	std::string totalSizeStr;
	if (noSFX) {
		std::cout << "!noSFX is enabled in asm/UserDefines.asm, sound effects are not included" << std::endl;
		totalSizeStr = "Total size of main program: 0x";
	}
	else {
		totalSizeStr = "Total size of main program + all sound effects: 0x";
	}
	std::cout << totalSizeStr << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << programSize  << std::dec << std::endl;

}

// TODO: Verify the working of "justSPCsPlease" and incorporate global songs my way.

void SPCEnvironment::compileMusic()
{
	if (verbose)
		std::cout << "Compiling music..." << std::endl;

	int totalSamplecount = 0;
	int totalSize = 0;
	int maxGlobalEchoBufferSize = 0;

	// Sets the maxGlobalEchoBufferSize as the maximum echo buffer size amongst
	// all global songs; and then, if each local song has an echo buffer size
	// lower than it, rise it accordingly.
	for (int i = 0; i < 256; i++)
	{
		if (musics[i].exists)
		{
			musics[i].index = i;
			if (i > highestGlobalSong) {
				musics[i].echoBufferSize = std::max(musics[i].echoBufferSize, maxGlobalEchoBufferSize);
			}
			musics[i].compile();
			if (i <= highestGlobalSong) {
				maxGlobalEchoBufferSize = std::max(musics[i].echoBufferSize, maxGlobalEchoBufferSize);
			}
			totalSamplecount += musics[i].mySamples.size();
		}
	}

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

	writeTextFile(build_dir / "SNES" / "SongSampleList.asm", s);
}

void SPCEnvironment::fixMusicPointers()
{
	if (verbose)
		std::cout << "Fixing song pointers..." << std::endl;

	int pointersPos = programSize + 0x400;
	std::stringstream globalPointers;
	std::stringstream incbins;

	int songDataARAMPos = programSize + programPos + highestGlobalSong * 2 + 2;
	//                    size + startPos + pointer to each global song + pointer to local song.
	//int songPointerARAMPos = programSize + programPos;

	bool addedLocalPtr = false;

	for (int i = 0; i < 256; i++)
	{
		if (musics[i].exists == false) continue;

		musics[i].posInARAM = songDataARAMPos;

		int untilJump = -1;

		if (i <= highestGlobalSong)
		{
			globalPointers << "\ndw song" << hex2 << i;
			incbins << "song" << hex2 << i << ": incbin \"" << "SNES/bin/" << "music" << hex2 << i << ".bin\"\n";
		}
		else if (addedLocalPtr == false)
		{
			globalPointers << "\ndw localSong";
			incbins << "localSong: ";
			addedLocalPtr = true;
		}

		for (int j = 0; j < musics[i].spaceForPointersAndInstrs; j+=2)
		{
			if (untilJump == 0)
			{
				j += musics[i].instrumentData.size();
				untilJump = -1;
			}

			int temp = musics[i].allPointersAndInstrs[j] | musics[i].allPointersAndInstrs[j+1] << 8;

			if (temp == 0xFFFF)		// 0xFFFF = swap with 0x0000.
			{
				musics[i].allPointersAndInstrs[j] = 0;
				musics[i].allPointersAndInstrs[j+1] = 0;
				untilJump = 1;
			}
			else if (temp == 0xFFFE)	// 0xFFFE = swap with 0x00FF.
			{
				musics[i].allPointersAndInstrs[j] = 0xFF;
				musics[i].allPointersAndInstrs[j+1] = 0;
				untilJump = 2;
			}
			else if (temp == 0xFFFD)	// 0xFFFD = swap with the song's position (its first track pointer).
			{
				musics[i].allPointersAndInstrs[j] = (musics[i].posInARAM + 2) & 0xFF;
				musics[i].allPointersAndInstrs[j+1] = (musics[i].posInARAM + 2) >> 8;
			}
			else if (temp == 0xFFFC)	// 0xFFFC = swap with the song's position + 2 (its second track pointer).
			{
				musics[i].allPointersAndInstrs[j] = musics[i].posInARAM & 0xFF;
				musics[i].allPointersAndInstrs[j+1] = musics[i].posInARAM >> 8;
			}
			else if (temp == 0xFFFB)	// 0xFFFB = swap with 0x0000, but don't set untilSkip.
			{
				musics[i].allPointersAndInstrs[j] = 0;
				musics[i].allPointersAndInstrs[j+1] = 0;
			}
			else
			{
				temp += musics[i].posInARAM;
				musics[i].allPointersAndInstrs[j] = temp & 0xFF;
				musics[i].allPointersAndInstrs[j+1] = temp >> 8;
			}
			untilJump--;
		}

		int normalChannelsSize = musics[i].data[0].size() + musics[i].data[1].size() + musics[i].data[2].size() +
			musics[i].data[3].size() + musics[i].data[4].size() + musics[i].data[5].size() +
			musics[i].data[6].size() + musics[i].data[7].size();

		for (int j = 0; j < 9; j++)
		{
			for (unsigned int k = 0; k < musics[i].loopLocations[j].size(); k++)
			{
				int temp = (musics[i].data[j][musics[i].loopLocations[j][k]] & 0xFF) | (musics[i].data[j][musics[i].loopLocations[j][k] + 1] << 8);
				temp += musics[i].posInARAM + normalChannelsSize + musics[i].spaceForPointersAndInstrs;
				musics[i].data[j][musics[i].loopLocations[j][k]] = temp & 0xFF;
				musics[i].data[j][musics[i].loopLocations[j][k] + 1] = temp >> 8;
			}
		}


		std::vector<uint8_t> final;

		int sizeWithPadding = (musics[i].minSize > 0) ? musics[i].minSize : musics[i].totalSize;

		if (i > highestGlobalSong)
		{
			int RATSSize = musics[i].totalSize + 4 - 1;
			final.push_back('S');
			final.push_back('T');
			final.push_back('A');
			final.push_back('R');

			final.push_back(RATSSize & 0xFF);
			final.push_back(RATSSize >> 8);

			final.push_back(~RATSSize & 0xFF);
			final.push_back(~RATSSize >> 8);

			final.push_back(sizeWithPadding & 0xFF);
			final.push_back(sizeWithPadding >> 8);

			final.push_back(songDataARAMPos & 0xFF);
			final.push_back(songDataARAMPos >> 8);
		}


		for (unsigned int j = 0; j < musics[i].allPointersAndInstrs.size(); j++)
			final.push_back(musics[i].allPointersAndInstrs[j]);

		for (unsigned int j = 0; j < 9; j++)
			for (unsigned int k = 0; k < musics[i].data[j].size(); k++)
				final.push_back(musics[i].data[j][k]);

		if (musics[i].minSize > 0 && i <= highestGlobalSong)
			while (final.size() < musics[i].minSize)
				final.push_back(0);


		if (i > highestGlobalSong)
		{
			musics[i].finalData.resize(final.size() - 12);
			musics[i].finalData.assign(final.begin() + 12, final.end());
		}

		std::stringstream fname;
		fname << "asm/SNES/bin/music" << hex2 << i << ".bin";
		writeFile(fname.str(), final);

		if (i <= highestGlobalSong)
		{
			songDataARAMPos += sizeWithPadding;
		}
		else
		{
			if (checkEcho)
			{
				musics[i].spaceInfo.songStartPos = songDataARAMPos;
				musics[i].spaceInfo.songEndPos = musics[i].spaceInfo.songStartPos + sizeWithPadding;

				int checkPos = songDataARAMPos + sizeWithPadding;
				if ((checkPos & 0xFF) != 0) checkPos = ((checkPos >> 8) + 1) << 8;

				musics[i].spaceInfo.sampleTableStartPos = checkPos;

				checkPos += musics[i].mySamples.size() * 4;

				musics[i].spaceInfo.sampleTableEndPos = checkPos;

				int importantSampleCount = 0;
				for (unsigned int j = 0; j < musics[i].mySamples.size(); j++)
				{
					auto thisSample = musics[i].mySamples[j];
					auto thisSampleSize = samples[thisSample].data.size();
					bool sampleIsImportant = samples[thisSample].important;
					if (sampleIsImportant) importantSampleCount++;

					musics[i].spaceInfo.individualSampleStartPositions.push_back(checkPos);
					musics[i].spaceInfo.individualSampleEndPositions.push_back(checkPos + thisSampleSize);
					musics[i].spaceInfo.individialSampleIsImportant.push_back(sampleIsImportant);

					checkPos += thisSampleSize;
				}
				musics[i].spaceInfo.importantSampleCount = importantSampleCount;

				if ((checkPos & 0xFF) != 0) checkPos = ((checkPos >> 8) + 1) << 8;

				//musics[i].spaceInfo.echoBufferStartPos = checkPos;

				checkPos += musics[i].echoBufferSize << 11;

				//musics[i].spaceInfo.echoBufferEndPos = checkPos;

				musics[i].spaceInfo.echoBufferEndPos = 0x10000;
				if (musics[i].echoBufferSize > 0)
				{
					musics[i].spaceInfo.echoBufferStartPos = 0x10000 - (musics[i].echoBufferSize << 11);
					musics[i].spaceInfo.echoBufferEndPos = 0x10000;
				}
				else
				{
					musics[i].spaceInfo.echoBufferStartPos = 0xFF00;
					musics[i].spaceInfo.echoBufferEndPos = 0xFF04;
				}


				if (checkPos > 0x10000)
				{
					std::cerr << musics[i].name << ": Echo buffer exceeded total space in ARAM by 0x" << hex4 << checkPos - 0x10000 << " bytes." << std::dec << std::endl;
					quit(1);
				}
			}
		}
	}

	//if (recompileMain)
	//{
	std::string patch;
	openTextFile("asm/tempmain.asm", patch);

	patch += globalPointers.str() + "\n" + incbins.str();

	writeTextFile("asm/tempmain.asm", patch);

	if (verbose)
		std::cout << "Compiling main SPC program, final pass." << std::endl;

	//removeFile("asm/SNES/bin/main.bin");

	//execute("asar asm/tempmain.asm asm/SNES/bin/main.bin 2> temp.log > temp.txt");

	//if (fileExists("temp.log"))
	if (!asarCompileToBIN("asm/tempmain.asm", "asm/SNES/bin/main.bin"))
		printError("asar reported an error while assembling asm/main.asm. Refer to temp.log for\ndetails.\n", true);
	//}

	programSize = getFileSize("asm/SNES/bin/main.bin");

	std::vector<uint8_t> temp;
	openFile("asm/SNES/bin/main.bin", temp);
	std::vector<uint8_t> temp2;
	temp2.resize(temp.size() + 4);
	temp2[0] = programSize & 0xFF;
	temp2[1] = programSize >> 8;
	temp2[2] = programPos & 0xFF;
	temp2[3] = programPos >> 8;
	for (unsigned int i = 0; i < temp.size(); i++)
		temp2[4+i] = temp[i];
	writeFile("asm/SNES/bin/main.bin", temp2);

	if (verbose)
		std::cout << "Total space in ARAM left for local songs: 0x" << hex4 << (0x10000 - programSize - 0x400) << " bytes." << std::dec << std::endl;

	int defaultIndex = -1, optimizedIndex = -1;
	for (unsigned int i = 0; i < bankDefines.size(); i++)
	{
		if (bankDefines[i]->name == "default")
			defaultIndex = i;
		if (bankDefines[i]->name == "optimized")
			optimizedIndex = i;
	}

}

void SPCEnvironment::generateSPCs()
{
	if (checkEcho == false)		// If echo buffer checking is off, then the overflow may be due to too many samples.
		return;			// In this case, trying to generate an SPC would crash.
	//uint8_t base[0x10000];

	std::vector<uint8_t> programData;
	openFile("asm/SNES/bin/main.bin", programData);
	programData.erase(programData.begin(), programData.begin() + 4);	// Erase the upload data.
	unsigned int i;

	unsigned int localPos;


	//for (i = 0; i < programPos; i++) base[i] = 0;

	//for (i = 0; i < programSize; i++)
	//	base[i + programPos] = programData[i];

	localPos = programData.size() + programPos;

	std::vector<uint8_t> SPC, SPCBase, DSPBase;
	openFile("asm/SNES/SPCBase.bin", SPCBase);
	openFile("asm/SNES/SPCDSPBase.bin", DSPBase);
	SPC.resize(0x10200);

	int SPCsGenerated = 0;

	bool forceAll = false;
	/*
	time_t recentMod = 0;			// If any main program modifications were made, we need to update all SPCs.
	for (int i = 1; i <= highestGlobalSong; i++)
		recentMod = std::max(recentMod, getTimeStamp((File)("music/" + musics[i].name)));

	recentMod = std::max(recentMod, getTimeStamp((File)"asm/main.asm"));
	recentMod = std::max(recentMod, getTimeStamp((File)"asm/commands.asm"));
	recentMod = std::max(recentMod, getTimeStamp((File)"asm/InstrumentData.asm"));
	recentMod = std::max(recentMod, getTimeStamp((File)"asm/CommandTable.asm"));
	recentMod = std::max(recentMod, getTimeStamp((File)"Addmusic_sound effects.txt"));
	recentMod = std::max(recentMod, getTimeStamp((File)"Addmusic_sample groups.txt"));
	recentMod = std::max(recentMod, getTimeStamp((File)"AddmusicK.exe"));

	for (int i = 1; i < 256; i++)
	{
		if (soundEffects[0][i].exists)
			recentMod = std::max(recentMod, getTimeStamp((File)((std::string)"1DF9/" + soundEffects[0][i].getEffectiveName())));
	}

	for (int i = 1; i < 256; i++)
	{
		if (soundEffects[1][i].exists)
			recentMod = std::max(recentMod, getTimeStamp((File)((std::string)"1DFC/" + soundEffects[1][i].getEffectiveName())));
	}
	*/
	int mode = 0;		// 0 = dump music, 1 = dump SFX1, 2 = dump SFX2
	int maxMode = 0;
	if (sfxDump == true) maxMode = 2;

	for (int mode = 0; mode <= maxMode; mode++)
	{

		for (unsigned int i = 0; i < 256; i++)
		{
			if (mode == 0 && musics[i].exists == false) continue;
			if (mode == 1 && soundEffects[0][i].exists == false) continue;
			if (mode == 2 && soundEffects[1][i].exists == false) continue;

			if (mode == 0 && i <= highestGlobalSong) continue;		// Cannot generate SPCs for global songs as required samples, SRCN table, etc. cannot be determined.


			//time_t spcTimeStamp = getTimeStamp((File)fname);

			/*if (!forceSPCGeneration)
				if (fileExists(fname))
				if (getTimeStamp((File)("music/" + musics[i].name)) < spcTimeStamp)
				if (getTimeStamp((File)"./samples") < spcTimeStamp)
				if (recentMod < spcTimeStamp)
				continue;*/

			SPCsGenerated++;
			int y = 2;					// Used to generate 2 SPCs for tracks with Yoshi drums.
			if (mode != 0) y = 1;
			while (y > 0)
			{
				if (mode == 0 && musics[i].hasYoshiDrums == false && y == 2)
				{
					y--;
					continue;
				}
				SPC.clear();
				SPC.resize(0x10200);

				for (unsigned int j = 0; j < SPCBase.size(); j++)
					SPC[j] = SPCBase[j];

				if (mode == 0) { for (unsigned int j = 0; j < 32; j++) if (j < musics[i].title.length())    SPC[j + 0x2E] = musics[i].title[j];    else SPC[j + 0x2E] = 0; }
				if (mode == 0) { for (unsigned int j = 0; j < 32; j++) if (j < musics[i].game.length())     SPC[j + 0x4E] = musics[i].game[j];     else SPC[j + 0x4E] = 0; }
				if (mode == 0) { for (unsigned int j = 0; j < 32; j++) if (j < musics[i].comment.length())  SPC[j + 0x7E] = musics[i].comment[j];  else SPC[j + 0x7E] = 0; }
				if (mode == 0) { for (unsigned int j = 0; j < 32; j++) if (j < musics[i].author.length())   SPC[j + 0xB1] = musics[i].author[j];   else SPC[j + 0xB1] = 0; }

				for (int j = 0; j < programSize; j++)
					SPC[0x100 + programPos + j] = programData[j];


				int backupIndex = i;
				if (mode != 0) {
					i = highestGlobalSong + 1;
					for (int j = highestGlobalSong+1; j < 256; j++) {
						if (musics[j].exists) {
							i = j;		// While dumping SFX, pretend that the current song is the lowest valid local song
							break;
						}
					}
				}

				if (mode == 0)
				{

					for (unsigned int j = 0; j < musics[i].finalData.size(); j++)
						SPC[localPos + 0x100 + j] = musics[i].finalData[j];


				}

				int tablePos = localPos + musics[i].finalData.size();

				if ((tablePos & 0xFF) != 0)
					tablePos = (tablePos & 0xFF00) + 0x100;

				int samplePos = tablePos + musics[i].mySamples.size() * 4;

				for (unsigned int j = 0; j < musics[i].mySamples.size(); j++)
				{
					SPC[tablePos + j * 4 + 0x100] = samplePos & 0xFF;
					SPC[tablePos + j * 4 + 0x101] = samplePos >> 8;
					unsigned short loopPoint = samples[musics[i].mySamples[j]].loopPoint;
					unsigned short newLoopPoint = loopPoint + samplePos;
					SPC[tablePos + j * 4 + 0x102] = newLoopPoint & 0xFF;
					SPC[tablePos + j * 4 + 0x103] = newLoopPoint >> 8;

					for (unsigned int k = 0; k < samples[musics[i].mySamples[j]].data.size(); k++)
						SPC[samplePos + 0x100 + k] = samples[musics[i].mySamples[j]].data[k];

					samplePos += samples[musics[i].mySamples[j]].data.size();
				}

				for (unsigned int j = 0; j < DSPBase.size(); j++)
					SPC[0x10100 + j] = DSPBase[j];

				SPC[0x1015D] = tablePos >> 8;

				SPC[0x15F] = 0x20;

				if (y == 2) SPC[0x01f5] = 2;

				SPC[0xA9] = (musics[i].seconds / 100 % 10) + '0';		// Why on Earth is the value stored as plain text...?
				SPC[0xAA] = (musics[i].seconds / 10 % 10) + '0';
				SPC[0xAB] = (musics[i].seconds / 1 % 10) + '0';

				SPC[0xAC] = '1';
				SPC[0xAD] = '0';
				SPC[0xAE] = '0';
				SPC[0xAF] = '0';
				SPC[0xB0] = '0';

				SPC[0x25] = mainLoopPos & 0xFF;	// Set the PC to the main loop.
				SPC[0x26] = mainLoopPos >> 8;	// The values of the registers (besides stack which is in the file) don't matter.  They're 0 in the base file.

				i = backupIndex;

				if (mode == 0)
				{
					SPC[0x1F6] = highestGlobalSong + 1;	// Tell the SPC to play this song.
				}
				else if (mode == 1)
				{
					SPC[0x1F4] = i;				// Tell the SPC to play this SFX
				}
				else if (mode == 2)
				{
					SPC[0x1F7] = i;				// Tell the SPC to play this SFX
				}




				char buffer[11];
				time_t t = time(NULL);
				strftime(buffer, 11, "%m/%d/%Y", localtime(&t));
				strncpy((char *)SPC.data() + 0x9E, buffer, 10);


				std::string pathlessSongName;
				if (mode == 0)
					pathlessSongName = musics[i].name;
				else if (mode == 1)
					pathlessSongName = soundEffects[0][i].name;
				else if (mode == 2)
					pathlessSongName = soundEffects[1][i].name;


				int extPos = pathlessSongName.find_last_of('.');
				if (extPos != -1)
					pathlessSongName = pathlessSongName.substr(0, extPos);


				if (pathlessSongName.find('/') != -1)
					pathlessSongName = pathlessSongName.substr(pathlessSongName.find_last_of('/') + 1);
				else if (pathlessSongName.find('\\') != -1)
					pathlessSongName = pathlessSongName.substr(pathlessSongName.find_last_of('\\') + 1);

				if (mode == 0) musics[i].pathlessSongName = pathlessSongName;
				if (mode == 1) pathlessSongName.insert(0, "1DF9/");
				if (mode == 2) pathlessSongName.insert(0, "1DFC/");

				std::string fname = "SPCs/" + pathlessSongName;


				if (y == 2)
					fname += " (Yoshi)";

				fname += ".spc";

				if (verbose)
					std::cout << "Wrote \"" << fname << "\" to file." << std::endl;

				writeFile(fname, SPC);
				y--;
			}

		}
	}

	if (verbose)
	{
		if (SPCsGenerated == 1)
			std::cout << "Generated 1 SPC file." << std::endl;
		else if (SPCsGenerated > 0)
			std::cout << "Generated " << SPCsGenerated << " SPC files." << std::endl;
	}
}

void SPCEnvironment::assembleSNESDriver2()
{
	if (verbose)
		std::cout << "\nGenerating SNES driver...\n" << std::endl;

	std::string patch;

	openTextFile("asm/SNES/patch.asm", patch);

	insertValue(reuploadPos, 4, "!ExpARAMRet = ", patch);
	insertValue(mainLoopPos, 4, "!DefARAMRet = ", patch);
	insertValue(songCount, 2, "!SongCount = ", patch);

	int pos;

	pos = patch.find("MusicPtrs:");
	if (pos == -1)
	{
		std::cerr << "Error: \"MusicPtrs:"" could not be found." << std::endl;		// // //
		quit(1);
	}

	patch = patch.substr(0, pos);

	{
		std::string patch2;
		openTextFile("asm/SNES/patch2.asm", patch2);
		patch += patch2;
	}

	std::stringstream musicPtrStr; musicPtrStr << "MusicPtrs: \ndl ";
	std::stringstream samplePtrStr; samplePtrStr << "\n\nSamplePtrs:\ndl ";
	std::stringstream sampleLoopPtrStr; sampleLoopPtrStr << "\n\nSampleLoopPtrs:\ndw ";
	std::stringstream musicIncbins; musicIncbins << "\n\n";
	std::stringstream sampleIncbins; sampleIncbins << "\n\n";

	if (verbose)
		std::cout << "Writing music files..." << std::endl;

	for (int i = 0; i < songCount; i++)
	{
		if (musics[i].exists == true && i > highestGlobalSong)
		{
			int requestSize;
			int freeSpace;
			std::stringstream musicBinPath;
			musicBinPath << "asm/SNES/bin/music" << hex2 << i << ".bin";
			requestSize = getFileSize(musicBinPath.str());
			freeSpace = findFreeSpace(requestSize, bankStart, rom);
			if (freeSpace == -1)
			{
				printError("Error: Your ROM is out of free space.", true);
			}

			freeSpace = PCToSNES(freeSpace);
			musicPtrStr << "music" << hex2 << i << "+8";
			musicIncbins << "org $" << hex6 << freeSpace << "\nmusic" << hex2 << i << ": incbin \"bin/music" << hex2 << i << ".bin\"" << std::endl;
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

	if (verbose)
		std::cout << "Writing sample files..." << std::endl;

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
			std::stringstream filename;
			filename << "asm/SNES/bin/brr" << hex2 << i << ".bin";
			writeFile(filename.str(), temp);

			int requestSize = getFileSize(filename.str());
			int freeSpace = findFreeSpace(requestSize, bankStart, rom);
			if (freeSpace == -1)
			{
				printError("Error: Your ROM is out of free space.", true);
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

	insertValue(highestGlobalSong, 2, "!GlobalMusicCount = #", patch);

	std::stringstream ss;
	ss << "\n\norg !SPCProgramLocation" << "\nincbin \"bin/main.bin\"";
	patch += ss.str();

	remove("asm/SNES/temppatch.sfc");

	std::string undoPatch;
	openTextFile("asm/SNES/AMUndo.asm", undoPatch);
	patch.insert(patch.begin(), undoPatch.begin(), undoPatch.end());

	writeTextFile("asm/SNES/temppatch.asm", patch);

	if (verbose)
		std::cout << "Final compilation..." << std::endl;

	if (!doNotPatch)
	{

		//execute("asar asm/SNES/temppatch.asm asm/SNES/temp.sfc 2> temp.log");
		//if (fileExists("temp.log"))
		if (!asarPatchToROM("asm/SNES/temppatch.asm", "asm/SNES/temp.sfc"))
			printError("asar reported an error.  Refer to temp.log for details.", true);

		//execute("asar asm/SNES/tweaks.asm asm/SNES/temp.sfc 2> temp.log");
		//if (fileExists("temp.log"))
		//	printError("asar reported an error.  Refer to temp.log for details.", true);

		//execute("asar asm/SNES/NMIFix.asm asm/SNES/temp.sfc 2> temp.log");
		//if (fileExists("temp.log"))
		//	printError("asar reported an error.  Refer to temp.log for details.", true);


		std::vector<uint8_t> final;
		final = romHeader;

		std::vector<uint8_t> tempsfc;
		openFile("asm/SNES/temp.sfc", tempsfc);

		for (unsigned int i = 0; i < tempsfc.size(); i++)
			final.push_back(tempsfc[i]);

		fs::remove((std::string)ROMName + "~");		// // //
		fs::rename((std::string)ROMName, (std::string)ROMName + "~");		// // //

		writeFile(ROMName, final);

	}
}

void SPCEnvironment::generateMSC()
{
	std::string mscname = ((std::string)ROMName).substr(0, (unsigned int)((std::string)ROMName).find_last_of('.'));

	mscname += ".msc";

	std::stringstream text;

	for (int i = 0; i < 256; i++)
	{
		if (musics[i].exists)
		{
			text << hex2 << i << "\t" << 0 << "\t" << musics[i].title << "\n";
			text << hex2 << i << "\t" << 1 << "\t" << musics[i].title << "\n";
			//fprintf(fout, "%2X\t0\t%s\n", i, musics[i].title.c_str());
			//fprintf(fout, "%2X\t1\t%s\n", i, musics[i].title.c_str());
		}
	}
	writeTextFile(mscname, text.str());
}

