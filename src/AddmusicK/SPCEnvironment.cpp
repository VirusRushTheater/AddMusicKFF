#include <cstring>

#include "AddmusicException.hpp"
#include "asarBinding.h"
#include "SPCEnvironment.h"
#include "Utility.h"

#include <iostream>

using namespace AddMusic;

// Will be evaluated relatively to work_dir to find source codes.
constexpr const char DEFAULT_SRC_FOLDER[] {"asm"};
constexpr const char DEFAULT_BUILD_FOLDER[] {"build"};

SPCEnvironment::SPCEnvironment(const fs::path& work_dir) :
	verbose(true),
	
	work_dir(work_dir),
	src_dir(work_dir / DEFAULT_SRC_FOLDER),	
	build_dir(work_dir / DEFAULT_BUILD_FOLDER)
{
	if (!fs::exists(src_dir))
		throw fs::filesystem_error("ASM source directory not found", src_dir, std::error_code());

	// Copies the Source directory into the build directory.
	copyDir(src_dir, build_dir);
}

void SPCEnvironment::loadSampleList(const fs::path& samplelist_file)
{
	std::string str;
	readTextFile(samplelist_file, str);

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

void SPCEnvironment::loadSFXList(const fs::path& sfxlist_file)
{
	std::string str;
	readTextFile(sfxlist_file, str);

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
	for (int i = 0; i < 256; i++)
	{
		if (musics[i].exists)
		{
			//if (!(i <= highestGlobalSong && !recompileMain))
			//{
			musics[i].index = i;
			if (i > highestGlobalSong) {
				musics[i].echoBufferSize = std::max(musics[i].echoBufferSize, maxGlobalEchoBufferSize);
			}
			musics[i].compile();
			if (i <= highestGlobalSong) {
				maxGlobalEchoBufferSize = std::max(musics[i].echoBufferSize, maxGlobalEchoBufferSize);
			}
			totalSamplecount += musics[i].mySamples.size();
			//}
		}
	}

	//int songSampleListSize = 0;

	//for (int i = 0; i < songCount; i++)
	//{
	//	songSampleListSize += musics[i].mySampleCount + 1;
	//}

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

	writeTextFile(build_dir / "SNES" / "SongSampleList.asm", s);
}