#include <cxxopts.hpp>
#include <iostream>

/**
 * @brief Convenient way to specify default arguments for the Addmusic class.
 * TODO: Put this into a header file
 */
struct Addmusic_arglist
{
	bool convert = true;
	bool checkEcho = true;
	bool forceSPCGeneration = false;
	int  bankStart = 0x200000;
	bool verbose = false;
	bool aggressive = false;
	bool dupCheck = true;
	bool validateHex = true;
	bool doNotPatch = false;
	int  errorCount = 0;
	bool optimizeSampleUsage = true;
	bool usingSA1 = false;
	bool allowSA1 = true;
	bool forceNoContinuePrompt = false;
	bool sfxDump = false;
	bool visualizeSongs = false;
	bool redirectStandardStreams = false;
	bool noSFX = false;

	bool preserveTemp = false;

	bool justSPCsPlease = false;
};

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
		("a,aggressive", "Aggressive", cxxopts::value<bool>())
		("b,bank_start", "Set bank start address on 0x080000", cxxopts::value<bool>())
		("c,convert", "NOT Convert", cxxopts::value<bool>())
		("d,dup_check", "NOT Dup check", cxxopts::value<bool>())
		("e,check_echo", "NOT Check echo", cxxopts::value<bool>())
		("p,do_not_patch", "Do not patch", cxxopts::value<bool>())
		("r,reset", "Reset Addmusic directory structure", cxxopts::value<bool>())
		("s,sa1", "Disable SA1 usage", cxxopts::value<bool>())
		("t,preserve", "Preserve temporarily generated files", cxxopts::value<bool>())
		("u,optimize_sp", "NOT Optimize sample usage", cxxopts::value<bool>())
		("v,verbose", "Verbose output", cxxopts::value<bool>())
		("x,validate_hex", "NOT Validate hex", cxxopts::value<bool>())
		("z,visualize", "Generate song visualization", cxxopts::value<bool>())

		("h,help", "Print usage", cxxopts::value<bool>())

		("dumpsfx", "Dump SFX", cxxopts::value<bool>())
		("norom", "Generate only SPCs")
		("version", "Program version")

		("romname", "ROM name", cxxopts::value<std::string>())
	;

	// The unnamed, positional argument will be the name of the rom to be patched.
	options.parse_positional({"romname"});
	auto argp = options.parse(argc, argv);

	// If the -h argument was passed, display help and quit.
	if (argp.count("help"))
    {
      	std::cout << options.help() << std::endl;
      	exit(0);
    }

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
		std::cout << "Rom name is supposed to be: " << argp["romname"].as<std::string>() << std::endl;
	}

	return 0;
}