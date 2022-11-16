#include <iostream>
#include <string>

#include <cxxopts.hpp>
#include "AddmusicK.hpp"

using namespace AddMusic;

/*
 * Only the argument passing mechanism will be implemented on the main function. 
*/
int main (int argc, char** argv)
{
	Addmusic_arglist am;

	// Using new CXXOpts parsing system.
	cxxopts::Options options("AddmusicK", "Rom hacking tool to insert custom music into a Super Mario World ROM");

	// TODO: Help me with better descriptions
	options.add_options()
		("a,aggressive", "Aggressive free space finding", cxxopts::value<bool>())
		("b,bank_start", "DISABLE bank optimizations", cxxopts::value<bool>())
		("c,convert", "DISABLE conversion for backwards compatibility", cxxopts::value<bool>())
		("d,dup_check", "DISABLE sample duplication check", cxxopts::value<bool>())
		("e,check_echo", "DISABLE echo buffer boundary check", cxxopts::value<bool>())
		("p,do_not_patch", "Do not patch the ROM, only generate a patch file", cxxopts::value<bool>())
		("r,reset", "Reset Addmusic directory structure", cxxopts::value<bool>())
		("s,sa1", "DISABLE SA-1 addressing", cxxopts::value<bool>())
		("t,preserve", "Preserve temporarily generated files", cxxopts::value<bool>())
		("u,optimize_sp", "DISABLE cleaning unused samples", cxxopts::value<bool>())
		("v,verbose", "Verbose output (also outputs the log in a .log file)", cxxopts::value<bool>())
		("w,work_folder", "Use another work folder", cxxopts::value<std::string>())
		("x,validate_hex", "DISABLE hex command validation", cxxopts::value<bool>())
		("z,visualize", "Generate song visualization", cxxopts::value<bool>())

		("dumpsfx", "Dump SFXs to the SPC folders", cxxopts::value<bool>())
		("norom", "Generate only SPCs")
		("noblock", "Continue even after an error")
		("version", "Program version")

		("romname", "ROM name", cxxopts::value<std::string>())

		("h,help", "Print help", cxxopts::value<bool>())
	;
	
	// The unnamed, positional argument will be the name of the rom to be patched.
	options.parse_positional({"romname"});
	auto argp = options.parse(argc, argv);

	// If the -h argument was passed, display help and quit.
	if (argp.count("help"))
    {
      	std::cout << options.help() << std::endl << std::endl;

		std::cout << "BASIC USAGE: \"" << argv[0] << " smwrom.smc\" will be enough in most cases." <<std::endl << std::endl;
		
		std::cout << "For a more detailed description of the command line arguments, please" << std::endl <<
		"read the \"Advanced Use\" readme file." << std::endl << std::endl;
      	exit(0);
    }

	// Work folder and Reset may work together.
	if (argp.count("work_folder"))
		am.workFolder =			argp["work_folder"].as<std::string>(); // new
	if (argp.count("reset"))
	{
		// TODO: Reset folder state
		exit(0);
	}

	if (argp.count("version"))
	{
		// TODO: Print program version
		exit(0);
	}

	// The rest of the options is parsed here.
	am.aggressive = 			argp["aggressive"].as<bool>();
	am.bankStart =				argp.count("bank_start") ? 0x080000 : 0x200000;
	am.convert = 				!argp["aggressive"].as<bool>();
	am.dupCheck = 				!argp["dup_check"].as<bool>();
	am.checkEcho = 				!argp["check_echo"].as<bool>();
	am.doNotPatch = 			argp["do_not_patch"].as<bool>();
	am.allowSA1 = 				!argp["sa1"].as<bool>();
	am.preserveTemp = 			argp["preserve"].as<bool>();	// new
	am.optimizeSampleUsage = 	!argp["optimize_sp"].as<bool>();
	am.verbose =				argp["verbose"].as<bool>();
	am.validateHex =			!argp["validate_hex"].as<bool>();
	am.visualizeSongs =			argp["visualize"].as<bool>();
	am.sfxDump =				argp["dumpsfx"].as<bool>();
	am.justSPCsPlease =			argp["norom"].as<bool>();
	am.forceNoContinuePrompt =	argp["noblock"].as<bool>();

	// Treatment depending on the presence of a ROM to be hacked.
	if (argp.count("romname") == 0)
	{
		std::cout << "No ROM file was specified." << std::endl << std::endl <<
		"If you want to only generate SPCs without patching a Super Mario World ROM," << std::endl <<
		"please use the --norom option." << std::endl;

		exit(1);
	}
	else
	{
		std::string rom_path = argp["romname"].as<std::string>();

		// Normal procedure to call the AddMusic instance and 
		// do the job in a very-high level.
		// AddMusicK amk_instance(am);
		// amk_instance.loadRom(rom_path);

		std::cout << "Rom name is supposed to be: " << rom_path << std::endl;
	}

	return 0;
}