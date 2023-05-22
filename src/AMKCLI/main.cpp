#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>

#include <cxxopts.hpp>
#include "defines.h"
#include "AddmusicK.h"

#include "SPCEnvironment.h"
#include "ROMEnvironment.h"

namespace fs = std::filesystem;

/**
 * Enum to tell whether this program will generate SPCs or patch a ROM.
 */
enum ProgramMode
{
	UNKNOWN, SPC_INDIVIDUAL_GENERATION, SPC_LIST_GENERATION, ROM_PATCHING
};

/**
 * Synthesis of the CLI options.
 */
struct CLIOptions
{
	ProgramMode mode;
	AddMusic::EnvironmentOptions spc_options;
};

int main (int argc, char** argv)
{
	CLIOptions o;

	// Parsing the CLI options.
	cxxopts::Options options(AMKNAME, "Toolkit to generate SPC files and patch SMW ROM music.");
	
	options.add_options("SPC generation")
		("m,mml", "Compile a MML file into SPC (can be set multiple times).", cxxopts::value<std::vector<std::string>>(), "<mml>")
		("visualize", "Plot local song memory usage", cxxopts::value<bool>()->default_value("false"));

	options.add_options("ROM patching")
		("r,rom", "Super Mario World ROM to be patched", cxxopts::value<std::string>(), "<path>")
		("l,list_folder", "AMK list folder", cxxopts::value<std::string>(), "<path>");

	options.add_options("Common");

	options.add_options("Advanced")
		("d,driver", "Custom SPC driver folder", cxxopts::value<std::string>(), "<path>")
		("aggressive", "Aggressive ROM space finding", cxxopts::value<bool>()->default_value("false"))
		("bankopt_off", "Turn off bank optimizations", cxxopts::value<bool>()->default_value("false"))
		("echocheck_off", "Turn off echo buffer bounds checking", cxxopts::value<bool>()->default_value("false"))
		("dupcheck_off", "Turn off sample duplicate checking", cxxopts::value<bool>()->default_value("false"))
		("sampleopt_off", "Turn off sample usage optimizations", cxxopts::value<bool>()->default_value("false"))
		("hexvalid_off", "Turn off hex command validation", cxxopts::value<bool>()->default_value("false"))
		("sa1_off", "Turn off SA1 addressing", cxxopts::value<bool>()->default_value("false"));

	options.add_options("Template extraction")
		("extract_lists", "Extract AMK lists template to a certain folder", cxxopts::value<std::string>(), "<path>")
		("extract_driver", "Extract SPC driver to a certain folder", cxxopts::value<std::string>(), "<path>");

	options.add_options()
		("v,version", "Program version")
		("y,yes", "Assume an affirmative answer to every user prompt.")
		("verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Print usage")
		("output", "Output folder/file", cxxopts::value<std::string>(), "<output>");

	options.parse_positional({"output"});
	auto argp = options.parse(argc, argv);

	// Print usage if either this program is called with no arguments or with the -h option.
	if (argc == 1 || argp.count("help"))
    {
		std::cout << options.help(std::vector<std::string> {"Common", "SPC generation", "ROM patching", "Advanced", "Template extraction", ""}) << std::endl;
		exit(0);
    }

	// Print version and quit.
	if (argp.count("version"))
	{
		std::cout << AMKNAME " v" AMKVER_FULL << std::endl;
		exit(0);
	}

	// Activate verbosity if this is the case.
	o.spc_options.verbose = (argp.count("verbose") > 0);

	// ==== Extraction ====
	// Extract any of the default packages and exit, if at least one of these options are set.
	if (argp.count("extract_lists"))
	{
		fs::path lists_folder = argp["extract_lists"].as<fs::path>();
		AddMusic::boilerplate_package.extract(lists_folder);
		std::cerr << "AMK lists template extracted at " << fs::absolute(lists_folder).string() << std::endl;
	}
	if (argp.count("extract_driver"))
	{
		fs::path asm_folder = argp["extract_driver"].as<fs::path>();
		AddMusic::asm_package.extract(asm_folder);
		std::cerr << "Default SPC driver extracted at " << fs::absolute(asm_folder).string() << std::endl;
	}
	if (argp.count("extract_lists") || argp.count("extract_driver"))
		exit(0);

	// ==== Advanced options ====
	// Parsing the custom driver folder, if any.
	if (argp.count("driver"))
	{
		fs::path driver_folder = fs::path(argp["driver"].as<std::string>());
		o.spc_options.useCustomSPCDriver = true;
		o.spc_options.customSPCDriverPath = driver_folder;
		std::cerr << "Using custom SPC driver at " << fs::absolute(driver_folder).string() << std::endl;
	}
	o.spc_options.aggressive = 			argp["aggressive"].as<bool>();
	o.spc_options.bankOptimizations = 	!argp["bankopt_off"].as<bool>();
	o.spc_options.checkEcho = 			!argp["echocheck_off"].as<bool>();
	o.spc_options.dupCheck = 			!argp["dupcheck_off"].as<bool>();
	o.spc_options.optimizeSampleUsage = !argp["sampleopt_off"].as<bool>();
	o.spc_options.validateHex = 		!argp["hexvalid_off"].as<bool>();
	o.spc_options.allowSA1 = 			!argp["sa1_off"].as<bool>();

	// Y/n prompt. Exits if "N" is answered.
	auto prompt = [argp](const std::string& msg)
	{
		char answer = ' ';
			
		while (true)
		{
			std::cout << msg << " (Y/n)" << std::endl;
			if (argp.count("yes"))
			{
				std::cout << "Y";
				return;
			}
			answer = std::getchar();
			if (std::tolower(answer) == 'n')
				exit(1);
			else if (std::tolower(answer) == 'y')
				break;
		}
	};

	// ==== ROM patching ====
	if (argp.count("rom"))
	{
		fs::path rom_location = fs::path(argp["rom"].as<std::string>());
		fs::path list_folder = (argp.count("list_folder")) ? fs::path(argp["list_folder"].as<std::string>()) : ".";
		fs::path output = (argp.count("output")) ? fs::path(argp["output"].as<std::string>()) : rom_location;

		if (!fs::exists(rom_location))
		{
			std::cerr << "The file " << rom_location << " does not exist." << std::endl;
			exit(1);
		}
		if (!fs::is_directory(list_folder))
		{
			std::cerr << "The folder " << rom_location << " does not exist." << std::endl;
			exit(1);
		}

		if (fs::equivalent(rom_location, output))
			prompt("The original ROM file will be overwritten. Is this ok?");
		else if (fs::exists(output))
			prompt("The specified output file will be overwritten. Is this ok?");
		
		// Instance a ROMEnvironment and make it work.
		AddMusic::ROMEnvironment rom_env (rom_location, list_folder, o.spc_options);
		rom_env.patchROM(output);

		std::cout << "Your ROM has been successfully patched and was stored at " << fs::absolute(output).string() << std::endl;
		exit(0);
	}

	// ==== MML compilation ====
	if (argp.count("mml"))
	{
		// If output (directory) is not specified, assume we're going to use the CWD.
		fs::path output = (argp.count("output")) ? fs::path(argp["output"].as<std::string>()) : fs::current_path();
		// This method is still lists-dependent (and mechanics such as samples must be copied like that as well). We'll change this soon.
		fs::path list_folder = (argp.count("list_folder")) ? fs::path(argp["list_folder"].as<std::string>()) : ".";

		std::vector<std::string> mml_list = argp["mml"].as<std::vector<std::string>>();
		std::vector<fs::path> mml_paths;
		for (auto& mml_i : mml_list)
		{
			if (!fs::exists(mml_i))
			{
				std::cerr << "The file " << mml_i << " does not exist." << std::endl;
				exit(1);
			}
			mml_paths.push_back(fs::path(mml_i));
		}

		// Instance a SPCEnvironment and make it work.
		AddMusic::SPCEnvironment spc_env (list_folder, o.spc_options);
		spc_env.generateSPCFiles(mml_paths, output);

		std::cout << "Your SPC files have been successfully generated and were stored at " << fs::absolute(output).string() << std::endl;
		exit(0);
	}
	
	return 0;
}