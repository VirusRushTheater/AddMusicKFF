#pragma once

#include <string>
#include <cstdlib>

// Local definitions
#include "Music.h"
#include "Sample.h"
#include "SoundEffect.h"
#include "SampleGroup.h"
#include "Directory.h"
#include "BankDefine.h"
#include "Directory.h"

// ASAR dependencies
#include <asar/interface-lib.h>

// Grab yer ducktape, folks!
#ifdef _WIN32
#define strnicmp _strnicmp
#else
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define AMKVERSION 		1
#define AMKMINOR 		0
#define AMKREVISION 	8			

#define PARSER_VERSION 	4			// Used to keep track of incompatible changes to the parser

#define DATA_VERSION 	0			// Used to keep track of incompatible changes to any and all compiled data, either to the SNES or to the PC

#define hex2 std::setw(2) << std::setfill('0') << std::uppercase << std::hex
#define hex4 std::setw(4) << std::setfill('0') << std::uppercase << std::hex
#define hex6 std::setw(6) << std::setfill('0') << std::uppercase << std::hex

#define REQUIRES_ROM_LOADED \
	if (rom.size() == 0) \


namespace AddMusic
{

typedef enum __Addmusic_error
{
	ADDMUSICERROR_SUCCESS = 0,		// No problem
	ADDMUSICERROR_ROM_NOT_LOADED,	// ROM not loaded
	ADDMUSICERROR_ASSERTION_FAILED,	// Assertion failed
	ADDMUSICERROR_ROM_TOO_SMALL,	// ROM is too small (<= 512 kB)
	ADDMUSICERROR_IO_ERROR,			// Filesystem error
	ADDMUSICERROR_NO_AM4_HEADER,	// No AddMusic 4.05 header
	ADDMUSICERROR_NO_AMM_HEADER,	// No AddMusicM header
	ADDMUSICERROR_NO_INIT_ASM,		// No INIT.asm to remove AddMusicM from the ROM.
} Addmusic_error;

// Error management routines
// TODO: Include the SPDLOG and FMT libraries.
#define __AMKERROR(STR,RETVAL,ISCRITICAL) \
	{ \
	printError(STR, ISCRITICAL); \
	last_error_details = STR; \
	if (ISCRITICAL) return(RETVAL); \
	}

#define AMKLOG(STR)					std::cout << STR << std::endl;
#define AMKWARNING(STR)				__AMKERROR(STR,RETVAL,false)
#define AMKERROR(STR,RETVAL)		__AMKERROR(STR,RETVAL,true)
#define AMKASSERT(CONDITION,STR)	\
	{ \
		if (!CONDITION) \
			AMKERROR(STR, ADDMUSICERROR_ASSERTION_FAILED); \
	}

#define AMKREQUIRESROM \
	{ \
		if (rom.size() == 0) \
			AMKERROR("This method requires a ROM to be loaded.", ADDMUSICERROR_ROM_NOT_LOADED);
	} \

/**
 * @brief Convenient way to specify, manipulate and pass default arguments
 * for the Addmusic class.
 */
struct Addmusic_arglist
{
	bool convert = true;						// Turn off conversion. The program changes certain things to make songs made with both Addmusic 4.05 and AddmusicM compatible with this program. Changes mostly include ignoring the header at the start of AM 4.05 songs, converting $ED $8X commands, etc. If this is not functioning correctly for some reason, use this argument to turn this behavior off.
	bool checkEcho = true;						// Turn off echo buffer bounds checking. By default the program will examine your songs, the samples they include, and the echo buffer sizes it uses. If it detects that their total is too large to fit into ARAM, it will give an error and stop. You can use this to turn that behavior off if it, for some reason, is not working. Turning this off, however, will disable SPC generation.
	int  bankStart = 0x200000;					// Turn off bank optimizations. By default, if your ROM is 4MB, the program will attempt to save data in banks $40+, so that ASM code can stay in the lower banks. This is normally what you want, but you can use this option to turn that off.
	bool verbose = false;						// Turn on verbosity. This will display extra information, like what the program is currently doing and how much space each channel of a song takes up.
	bool aggressive = false;					// Turn on aggressive free space finding. If this is on, all data not protected by a RATS tag beyond SMW's normal data boundary is fair game. Using this is not necessarily recommended due to the fact that it is perfectly reasonable to expect that there is data in a ROM accidentally not RATS protected, but if you really need the extra space, and have nothing to lose, go for it.
	bool dupCheck = true;						// Turns sample duplicate checking for samples. Extremely useless, as turning this on means that each song has its own unique set of samples and an enormous amount of space will be wasted. This used to be only for sample bank samples, but now it's pretty much just here for fun. Turning this on will basically eat up all your freespace (though you can reverse the damage by just running the program again without this option).
	bool validateHex = true;					// Turn off hex command validation. Use this if you're defining your own hex commands in the SPC program or, if for whatever reason, the normal validation system that the program uses isn't working. Normally the program attempts to validate hex command input and will give an error if it comes across something invalid.
	bool doNotPatch = false;					// Do not modify the ROM; only generate patches, .bin files, .spc files, etc. Also turns off the cleaning up of temporary files. You may find the generated patch in "asm/SNES/temppatch.asm"; patching that to the ROM with asar will accomplish the same thing as not using this option.
	bool optimizeSampleUsage = true;			// Turn off sample optimizations. Normally, the program will not insert any samples into a song that are not used (barring "important" samples). If you specify this option, that behavior will be turned off.
	bool allowSA1 = true;						// Turn off SA-1 addressing. By default, if $00FFD5 in the ROM is $23, then the program will use SA-1 addressing for the generated patch. If this option is used, that behavior will be disabled. Note that this will only affect addressing used for generated features; for the full effect, you must make some minor changes in asm/SNES/patch.asm.
	bool forceNoContinuePrompt = false;			// Normally if AMK encounters an error while running, it will display the error(s) and then wait for the user to press the enter key to continue. This flag will turn this behavior off, so on failure the program will simply quit.
	bool sfxDump = false;						// Dumps all sound effects to the SPC folder inside their respective SFX directories. Note the samples used will be from the song you specify or, when modifying a ROM, the lowest numbered local song. Please make sure !noSFX is set to !false in asm/UserDefines.asm: otherwise, this option will not work.
	bool visualizeSongs = false;				// Creates a series of PNG files for the memory usage of local song(s). Each PNG contains a set of color strips, 16 bytes per column, that contains a certain memory usage data.

	std::string workFolder = "./";				// Work folder where your song structure is stored.
	bool preserveTemp = false;					// Preserve the temporary files generated in the process. Normally developers will be interested in this.

	bool justSPCsPlease = false;				// Only do what's necessary to generate SPC files; this makes it possible to generate SPCs without a Super Mario World ROM. After using this option, you must specify the files you wish to compile (with quotes if they contain spaces). For example, -norom test.txt "test2.txt". Please note that global songs and sound effects must still be parsed, so Addmusic_list.txt, Addmusic_sound effects.txt, and Addmusic_sample groups.txt must all be valid.
};

class AddMusicK
{
	public:
	/**
	 * @brief Initializes an AddMusicK instance with the default arguments.
	 */
	AddMusicK()
	{
	}

	/**
	 * @brief Initializes an AddMusicK instance with given arguments, as to
	 * emulate the CLI parameters it used to have.
	 */
	AddMusicK(Addmusic_arglist addmusic_args) :
		p(addmusic_args)
	{
	}

	/**
	 * @brief Loads a Super Mario World ROM from the file system, preparing it
	 * to apply a music patch. The ROM file is not affected.
	 * 
	 * @return
	 * ADDMUSICERROR_SUCCESS if the ROM was successfully loaded,
	 * ADDMUSICERROR_ROM_TOO_SMALL if the ROM needs to be expanded (is 512kB)
	 * ADDMUSICERROR_IO_ERROR if the file does not exist or is not readable.
	 */
	Addmusic_error loadRom(std::string rom_path);

	~AddMusicK()
	{
	}

	/**
	 * @brief Tries to clear any patches made by AddMusic 4.05. Requires a ROM
	 * to be loaded.
	 * 
	 * @return 
	 * ADDMUSICERROR_SUCCESS if the ROM was successfully cleared,
	 * ADDMUSICERROR_NO_AM4_HEADER if the ROM has no AddMusic 4.05 header.
	 */
	Addmusic_error tryToCleanAM4Data();

	/**
	 * @brief Tries to clear any patches made by AddMusicM. Requires a ROM
	 * to be loaded.
	 * 
	 * @return 
	 * ADDMUSICERROR_SUCCESS if the ROM was successfully cleared,
	 * ADDMUSICERROR_NO_AMM_HEADER if the ROM has no AddMusic 4.05 header.
	 */
	Addmusic_error tryToCleanAMMData();

	/**
	 * @brief Cleans the Super Mario World ROM of info from any modern AMK
	 * utility. Requires a ROM to be loaded.
	 * 
	 * @return
	 * ADDMUSICERROR_SUCCESS if the ROM was successfully cleared,
	 */
	Addmusic_error cleanROM();

	/**
	 * @brief Cleans the Super Mario World ROM of info data.
	 * 
	 * @return
	 * ADDMUSICERROR_SUCCESS if the ROM was successfully cleared,
	 */
	Addmusic_error tryToCleanSampleToolData();

	

	File ROMName;					// ROM name in the file system
	std::vector<uint8_t> rom;		// ROM in memory
	std::vector<uint8_t> romHeader;
	std::string tempDir;			// Temporary directory path. To be used by compilation purposes.

	private:
	Addmusic_arglist p;
	std::string last_error_details = "";	// Last error details

	bool usingSA1;
};
}
