#include <cstring>
#include <algorithm>

#include "AddmusicLogging.h"
#include "asarBinding.h"
#include "SPCEnvironment.h"
#include "Utility.h"
#include "Package.h"

#include <iostream>

using namespace AddMusic;

SPCEnvironment::SPCEnvironment(const fs::path& work_dir, EnvironmentOptions opts) :
	work_dir(work_dir),
	global_samples_dir(work_dir / "samples"),
	driver_srcdir(std::filesystem::temp_directory_path() / "amkdriver"),	// /tmp/amkdriver in Linux
	driver_builddir(driver_srcdir)
{
	options = opts;

	// Set another directory for samples, if specified here.
	if (!options.customSamplesPath.empty())
		global_samples_dir = options.customSamplesPath;

	// Delegate these "if (verbose)" clauses to this Logging singleton.
	if (options.verbose)
		Logging::setVerbosity(Logging::Levels::DEBUG);

	// If we don't have an empty custom SPC driver path, change our driver_srcdir and driver_builddir
	using_custom_spc_driver = options.useCustomSPCDriver && !options.customSPCDriverPath.empty();
	if (using_custom_spc_driver)
	{
		// Use that custom driver and create a "build" directory on it.
		driver_srcdir = options.customSPCDriverPath;
		driver_builddir = driver_srcdir / "build";

		// If the "build" directory exists, remove it and copy the driver entirely into it.
		// It won't be deleted after the program exits.
		if (fs::exists(driver_builddir))
			deleteDir(driver_builddir);
		copyDir(driver_srcdir, driver_builddir);

		Logging::debug("Using custom SPC driver at " + options.customSPCDriverPath.string());
	}
	else
	{
		// Extract the embedded ASM driver into a temporary folder.
		asm_package.extract(driver_srcdir);
		Logging::debug("Extracting the embedded SPC driver into a temporary folder.");
	}

	if (!options.allowSA1)
		usingSA1 = false;
	
	// Does your work directory have these files?
	if (!fs::exists(work_dir))
		throw fs::filesystem_error("The directory with music has not been found", work_dir, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SONGLIST_FILENAME))
		throw fs::filesystem_error("The song list file was not found within the work directory.", work_dir / DEFAULT_SONGLIST_FILENAME, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SAMPLELIST_FILENAME))
		throw fs::filesystem_error("The sample list file was not found within the work directory.", work_dir / DEFAULT_SAMPLELIST_FILENAME, std::error_code());
	if (!fs::exists(work_dir / DEFAULT_SFXLIST_FILENAME))
		throw fs::filesystem_error("The SFX list file was not found within the work directory.", work_dir / DEFAULT_SFXLIST_FILENAME, std::error_code());
	
	
	// Dynamic allocation of some arrays.
	musics = new Music[256];
	soundEffectsDF9 = new SoundEffect[256];
	soundEffectsDFC = new SoundEffect[256];
	soundEffects[0] = soundEffectsDF9;
	soundEffects[1] = soundEffectsDFC;

	Logging::debug(std::string("Driver will be compiled in ") + fs::absolute(driver_builddir).string());
}

SPCEnvironment::~SPCEnvironment()
{
	delete[] (musics);
	delete[] (soundEffectsDF9);
	delete[] (soundEffectsDFC);

	// If using an extracted driver, delete such temporary folder.
	if (!using_custom_spc_driver)
		deleteDir(driver_srcdir);
}

bool SPCEnvironment::generateSPCFiles(const std::vector<fs::path>& textFilesToCompile, const fs::path& output_folder)
{
	justSPCsPlease = true;
	spc_output_dir = output_folder;
	spc_build_plan = true;

	loadSampleList(work_dir / DEFAULT_SAMPLELIST_FILENAME);
	loadMusicList(work_dir / DEFAULT_SONGLIST_FILENAME);
	loadSFXList(work_dir / DEFAULT_SFXLIST_FILENAME);

	_assembleSNESDriver();		// We need this for the upload position, where the SPC file's PC starts.  Luckily, this function is very short.

	_assembleSPCDriver();
	_compileSFX();
	_compileGlobalData();

	// We start loading CLI songs from highestGlobalSong + 1. If no global songs are
	// present, highestGlobalSong = 0 and we start loading songs from slot 1. We
	// leave slot 0 empty, to match the SNES driver which treats song 0 as a NOP rather
	// than a song number, with the song ID also being unsendable and unplayable on the
	// SPC under its raw ID because that also acts as a NOP.
	int firstLocalSong = highestGlobalSong + 1;

	// Unset local songs loaded from Addmusic_list.txt.
	for (int i = firstLocalSong; i < 256; i++)
		musics[i].exists = false;

	// Load local songs from command-line arguments.
	for (int i = firstLocalSong, j = 0; (i < 256) && (j < textFilesToCompile.size()); i++, j++)
	{
		if (i >= 256)
			Logging::error("Error: The total number of requested music files to compile exceeded 255.");
		musics[i].exists = true;
		musics[i].name = textFilesToCompile[j];
	}

	for (int i = 0; i < 256; i++)
	{
		// Load music file in memory.
		if (musics[i].exists)
			readTextFile(fs::absolute(musics[i].name), musics[i].text);
	}

	_compileMusic();
	_fixMusicPointers();

	_generateSPCs();
	spc_build_plan = false;

	return true;
}

bool SPCEnvironment::_assembleSNESDriver()
{
	std::string patch;
	readTextFile(driver_builddir / "SNES" / "patch.asm", patch);
	programUploadPos = scanInt(patch, "!DefARAMRet = ");

	return true;
}

// Equivalent to assembleSPCDriver()
bool SPCEnvironment::_assembleSPCDriver()
{
	std::string patch;
	readTextFile(driver_builddir / "main.asm", patch);
	programPos = scanInt(patch, "base ");

	// Everything is done through memory this time.
	bool firstpass_success;
	AsarBinding firstpass (driver_builddir / "main.asm");
	firstpass_success = firstpass.compileToBin();
	if (!firstpass_success)
	{
		firstpass.printErrors();
		Logging::error("asar reported an error while assembling asm/main.asm.");
		return false;
	}

	// Scans the messages deployed by the assembly process.
	std::string firstpass_stdout = firstpass.getStdout();

	mainLoopPos = scanInt(firstpass_stdout, "MainLoopPos: ");
	reuploadPos = scanInt(firstpass_stdout, "ReuploadPos: ");
	noSFX = (firstpass_stdout.find("NoSFX is enabled") != -1);
	programSize = firstpass.getProgramSize();

	if (options.sfxDump && noSFX) {
		Logging::warning("The sound driver build does not support sound effects due to the !noSFX flag being enabled in asm/UserDefines.asm, yet you requested to dump SFX. There will be no new SPC dumps of the sound effects since the data is not included by default, nor is the playback code for the sound effects.");
		options.sfxDump = false;
		return false;
	}

	return true;
}

bool SPCEnvironment::_compileSFX()
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
					Logging::error(std::string("Error: The sound effect that sound effect 0x") + r.str() + std::string(" points to could not be found."));
					return false;
				}
			}
		}
	}
	return true;
}

bool SPCEnvironment::_compileGlobalData()
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
			soundEffects[0][i].compile(this);
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
			{
				Logging::error("Error: A sound effect that is a pointer to another sound effect must come after\nthe sound effect that it points to.");
				return false;
			}
		}
	}

	// if (errorCount > 0)
	// 	Logging::error("There were errors when compiling the sound effects.  Compilation aborted.  Your\nROM has not been modified.");

	for (int i = 0; i <= DFCCount; i++)
	{
		if (soundEffects[1][i].exists && soundEffects[1][i].pointsTo == 0)
		{
			soundEffects[1][i].posInARAM = DFCCount * 2 + DF9Count * 2 + programPos + programSize + DF9DataTotal + DFCDataTotal;
			soundEffects[1][i].compile(this);
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
			{
				Logging::error("Error: A sound effect that is a pointer to another sound effect must come after\nthe sound effect that it points to.");
				return false;
			}
		}
	}

	// if (errorCount > 0)
	// 	Logging::error("There were errors when compiling the sound effects.  Compilation aborted.  Your\nROM has not been modified.");

	Logging::debug(std::stringstream() << "Total space used by 1DF9 sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DF9DataTotal + DF9Count * 2) << std::dec);
	Logging::debug(std::stringstream() << "Total space used by 1DFC sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DFCDataTotal + DFCCount * 2) << std::dec);

	Logging::info(std::stringstream() << "Total space used by all sound effects: 0x" << std::hex << std::setw(4) << std::uppercase << std::setfill('0') << (DF9DataTotal + DF9Count * 2 + DFCDataTotal + DFCCount * 2) << std::dec);

	DF9Pointers.erase(DF9Pointers.begin(), DF9Pointers.begin() + 1);
	DFCPointers.erase(DFCPointers.begin(), DFCPointers.begin() + 1);

	writeBinaryFile(driver_builddir / "SFX1DF9Table.bin", DF9Pointers);
	writeBinaryFile(driver_builddir / "SFX1DFCTable.bin", DFCPointers);

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

	writeBinaryFile(driver_builddir / "SFXData.bin", allSFXData);

	std::string str;
	readTextFile(driver_builddir / "main.asm", str);

	int pos;

	pos = str.find("SFXTable0:");
	if (pos == -1) Logging::error("Error: SFXTable0 not found in main.asm.");
	str.insert(pos+10, "\r\nincbin \"SFX1DF9Table.bin\"\r\n");

	pos = str.find("SFXTable1:");
	if (pos == -1) Logging::error("Error: SFXTable1 not found in main.asm.");
	str.insert(pos+10, "\r\nincbin \"SFX1DFCTable.bin\"\r\nincbin \"SFXData.bin\"\r\n");

	writeTextFile(driver_builddir / "tempmain.asm", str);
	AsarBinding asar2 (driver_builddir / "tempmain.asm");

	Logging::debug("Compiling main SPC program, pass 2.");

	if (!asar2.compileToFile(driver_builddir / "main.bin"))
	{
		asar2.printErrors();
		Logging::error("asar reported an error while assembling asm/main.asm. Refer to temp.log for\ndetails.\n");
		return false;
	}

	programSize = asar2.getProgramSize();

	std::string totalSizeStr;
	if (noSFX) {
		Logging::info("!noSFX is enabled in asm/UserDefines.asm, sound effects are not included");
		totalSizeStr = "Total size of main program: 0x";
	}
	else {
		totalSizeStr = "Total size of main program + all sound effects: 0x";
	}
	std::cout << totalSizeStr << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << programSize << std::dec << std::endl;

	return true;
}

bool SPCEnvironment::_compileMusic()
{
	Logging::debug("Compiling music...");

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
			musics[i].compile(this);
			if (i <= highestGlobalSong) {
				maxGlobalEchoBufferSize = std::max(musics[i].echoBufferSize, maxGlobalEchoBufferSize);
			}
			totalSamplecount += musics[i].mySamples.size();
			//}
		}
	}

	return true;
}

bool SPCEnvironment::_fixMusicPointers()
{
	Logging::debug("Fixing song pointers...");

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

		int normalChannelsSize = musics[i].data[0].size() + musics[i].data[1].size() + musics[i].data[2].size() + musics[i].data[3].size() + musics[i].data[4].size() + musics[i].data[5].size() + musics[i].data[6].size() + musics[i].data[7].size();

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
		fs::path globalinc_name (driver_builddir / "SNES" / "bin" / (std::stringstream() << "music" << hex2 << i << ".bin").str());
		writeBinaryFile(globalinc_name, final);

		if (i <= highestGlobalSong)
		{
			songDataARAMPos += sizeWithPadding;
		}
		else
		{
			if (options.checkEcho)
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
					Logging::error(std::stringstream() << musics[i].name << ": Echo buffer exceeded total space in ARAM by 0x" << hex4 << checkPos - 0x10000 << " bytes." << std::dec);
					return false;
				}
			}
		}
	}

	//if (recompileMain)
	//{
	std::string patch;
	readTextFile(driver_builddir / "tempmain.asm", patch);

	patch += globalPointers.str() + "\n" + incbins.str();

	writeTextFile(driver_builddir / "tempmain.asm", patch);

	Logging::debug("Compiling main SPC program, final pass.");

	AsarBinding asar3 (driver_builddir / "tempmain.asm");
	if (!asar3.compileToFile(driver_builddir / "SNES" / "bin" / "main.bin"))
	{
		asar3.printErrors();
		Logging::error("asar reported an error while assembling asm/main.asm.");
		return false;
	}

	programSize = asar3.getProgramSize();

	std::vector<uint8_t> temp;
	readBinaryFile(driver_builddir / "SNES" / "bin" / "main.bin", temp);
	std::vector<uint8_t> temp2;
	temp2.resize(temp.size() + 4);
	temp2[0] = programSize & 0xFF;
	temp2[1] = programSize >> 8;
	temp2[2] = programPos & 0xFF;
	temp2[3] = programPos >> 8;
	for (unsigned int i = 0; i < temp.size(); i++)
		temp2[4+i] = temp[i];
	writeBinaryFile(driver_builddir / "SNES" / "bin" / "main.bin", temp2);

	Logging::debug(std::stringstream() << "Total space in ARAM left for local songs: 0x" << hex4 << (0x10000 - programSize - 0x400) << " bytes." << std::dec);

	int defaultIndex = -1, optimizedIndex = -1;
	for (unsigned int i = 0; i < bankDefines.size(); i++)
	{
		if (bankDefines[i]->name == "default")
			defaultIndex = i;
		if (bankDefines[i]->name == "optimized")
			optimizedIndex = i;
	}

	return true;
}

bool SPCEnvironment::_generateSPCs()
{
	if (options.checkEcho == false)		// If echo buffer checking is off, then the overflow may be due to too many samples.
		return false;			// In this case, trying to generate an SPC would crash.
	//uint8_t base[0x10000];

	std::vector<uint8_t> programData;
	readBinaryFile(driver_builddir / "SNES" / "bin" / "main.bin", programData);
	programData.erase(programData.begin(), programData.begin() + 4);	// Erase the upload data.
	unsigned int i;

	unsigned int localPos;


	//for (i = 0; i < programPos; i++) base[i] = 0;

	//for (i = 0; i < programSize; i++)
	//	base[i + programPos] = programData[i];

	localPos = programData.size() + programPos;

	std::vector<uint8_t> SPC, SPCBase, DSPBase;
	readBinaryFile(driver_builddir / "SNES" / "SPCBase.bin", SPCBase);
	readBinaryFile(driver_builddir / "SNES" / "SPCDSPBase.bin", DSPBase);
	SPC.resize(0x10200);

	int SPCsGenerated = 0;

	bool forceAll = false;

	int mode = 0;		// 0 = dump music, 1 = dump SFX1, 2 = dump SFX2
	int maxMode = 0;
	if (options.sfxDump == true) maxMode = 2;

	for (int mode = 0; mode <= maxMode; mode++)
	{

		for (unsigned int i = 0; i < 256; i++)
		{
			if (mode == 0 && musics[i].exists == false) continue;
			if (mode == 1 && soundEffects[0][i].exists == false) continue;
			if (mode == 2 && soundEffects[1][i].exists == false) continue;

			if (mode == 0 && i <= highestGlobalSong) continue;		// Cannot generate SPCs for global songs as required samples, SRCN table, etc. cannot be determined.

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

				fs::path pathlessSongName;
				if (mode == 0)
					pathlessSongName = musics[i].name.stem();
				else if (mode == 1)
					pathlessSongName = soundEffects[0][i].name.stem();
				else if (mode == 2)
					pathlessSongName = soundEffects[1][i].name.stem();

				if (mode == 0) musics[i].pathlessSongName = pathlessSongName.string();
				if (mode == 1) {
					pathlessSongName = "1DF9" / pathlessSongName;
					if (!fs::exists("1DF9"))
						fs::create_directory(spc_output_dir / "1DF9");
				}
				if (mode == 2) {
					pathlessSongName = "1DFC" / pathlessSongName;
					if (!fs::exists("1DFC"))
						fs::create_directory(spc_output_dir / "1DFC");
				}

				fs::path fname = spc_output_dir / pathlessSongName;

				if (y == 2)
					fname += " (Yoshi)";

				fname += ".spc";

				Logging::debug(std::string("Wrote \"") + fname.string() + "\" to file.");

				// Hotfix to not store SPCs if we're patching a ROM.
				if (spc_build_plan)
					writeBinaryFile(fname, SPC);
				y--;
			}

		}
	}

	Logging::debug(std::string("Generated ") + std::to_string(SPCsGenerated) + " SPC file(s)");
	return true;
}

void SPCEnvironment::loadSampleList(const fs::path& samplelistfile)
{
	std::string str;
	readTextFile(samplelistfile, str);

	std::string groupName;
	std::string tempName;

	unsigned int i = 0;
	bool gettingGroupName = false;
	bool gettingSampleName = false;

	// Important routines in parsers will be put as lambdas here.
	auto createSampleGroup = [](const std::string& group_name) -> void {

	};

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
					Logging::error("Error parsing sample groups.txt.  Expected opening curly brace.");
				}
			}
		}
		else if (gettingGroupName == true)
		{
			if (isspace(str[i]))
			{
				// Create a bank definition and give it a name.
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
						Logging::error("Error parsing Addmusic_sample groups.txt: Importance specifier ('!') must come after asample declaration, not before it.");
					bankDefines[bankDefines.size() - 1]->importants[bankDefines[bankDefines.size() - 1]->importants.size() - 1] = true;
					i++;
				}
				else
				{
					Logging::error("Error parsing sample groups.txt.  Expected opening quote.");
				}
			}
		}
	}
}

void SPCEnvironment::loadMusicList(const fs::path& musiclistfile)
{
	std::string musicFile;
	readTextFile(musiclistfile, musicFile);

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
			Logging::error("Error: Could not find \"Globals:\" label in list.txt");

		if (index < 0)
		{
			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index = musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index = musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index = musicFile[i++] - 'a' + 10;
			else Logging::error("Invalid number in list.txt.");

			index <<= 4;


			if      ('0' <= musicFile[i] && musicFile[i] <= '9') index |= musicFile[i++] - '0';
			else if ('A' <= musicFile[i] && musicFile[i] <= 'F') index |= musicFile[i++] - 'A' + 10;
			else if ('a' <= musicFile[i] && musicFile[i] <= 'f') index |= musicFile[i++] - 'a' + 10;
			else if (isspace(musicFile[i])) index >>= 4;
			else Logging::error("Invalid number in list.txt.");

			if (!isspace(musicFile[i]))
				Logging::error("Invalid number in list.txt.");
			if (inGlobals)
				highestGlobalSong = std::max(highestGlobalSong, index);
			if (inLocals)
				if (index <= highestGlobalSong)
					Logging::error("Error: Local song numbers must be greater than the largest global song number.");
		}
		else
		{
			if (musicFile[i] == '\n' || musicFile[i] == '\r')
			{
				musics[index].name = work_dir / "music" / tempName;
				// Don't bother loading local songs if we're just going to compile SPCs on demand.
				if (inLocals && justSPCsPlease == false)
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

	Logging::verbose(std::string("Read in all ") + std::to_string(shallowSongCount) + " songs.");

	for (int i = 255; i >= 0; i--)
	{
		if (musics[i].exists)
		{
			songCount = i+1;
			break;
		}
	}
}

void SPCEnvironment::loadSFXList(const fs::path& sfxlistfile)
{	std::string str;
	readTextFile(sfxlistfile, str);

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
			Logging::error("Error: Could not find \"SFX1DF9:\" label in sound effects.txt");

		if (index < 0)
		{
			if      ('0' <= str[i] && str[i] <= '9') index = str[i++] - '0';
			else if ('A' <= str[i] && str[i] <= 'F') index = str[i++] - 'A' + 10;
			else if ('a' <= str[i] && str[i] <= 'f') index = str[i++] - 'a' + 10;
			else Logging::error("Invalid number in sound effects.txt.");

			index <<= 4;


			if      ('0' <= str[i] && str[i] <= '9') index |= str[i++] - '0';
			else if ('A' <= str[i] && str[i] <= 'F') index |= str[i++] - 'A' + 10;
			else if ('a' <= str[i] && str[i] <= 'f') index |= str[i++] - 'a' + 10;
			else if (isspace(str[i])) index >>= 4;
			else Logging::error("Invalid number in sound effects.txt.");


			if (!isspace(str[i]))
				Logging::error("Invalid number in sound effects.txt.");
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
							soundEffects[0][index].name = tempName;
						else
							soundEffects[0][index].pointName = tempName;

						soundEffects[0][index].exists = true;

						if (doNotAdd0)
							soundEffects[0][index].add0 = false;
						else
							soundEffects[0][index].add0 = true;

						if (!isPointer)
							readTextFile(work_dir / "1DF9" / tempName, soundEffects[0][index].text);
					}
					else
					{
						if (!isPointer)
							soundEffects[1][index].name = tempName;
						else
							soundEffects[1][index].pointName = tempName;

						soundEffects[1][index].exists = true;

						if (doNotAdd0)
							soundEffects[1][index].add0 = false;
						else
							soundEffects[1][index].add0 = true;

						if (!isPointer)
							readTextFile(work_dir / "1DFC" / tempName, soundEffects[1][index].text);
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

	Logging::debug(std::string("Read in all ") + std::to_string(SFXCount) + " sound effects.");
}

int SPCEnvironment::SNESToPC(int addr)					// Thanks to alcaro.
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

int SPCEnvironment::PCToSNES(int addr)
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

