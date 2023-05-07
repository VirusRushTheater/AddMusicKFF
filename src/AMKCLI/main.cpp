#include <string>
#include <iostream>
#include <filesystem>

#include <cxxopts.hpp>
#include "AddmusicK.h"

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
	AddMusic::SPCEnvironmentOptions spc_options;
};

int main (int argc, char** argv)
{
	CLIOptions o;

	// Parsing the CLI options.
	cxxopts::Options options("AddmusicK", "Toolkit to generate SPC files and patch SMW ROM music.");
	options.add_options()
		("d,driver", "Custom SPC driver folder", cxxopts::value<fs::path>())
		("l,list_folder", "AMK list folder", cxxopts::value<fs::path>())
		("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))

		("aggressive", "Aggressive ROM space finding", cxxopts::value<bool>()->default_value("false"))
		("sa1_off", "Turn off SA1 addressing", cxxopts::value<bool>()->default_value("false"))
		("echocheck_off", "Turn off echo buffer bounds checking", cxxopts::value<bool>()->default_value("false"))
		("dupcheck_off", "Turn off sample duplicate checking", cxxopts::value<bool>()->default_value("false"))

		("extract_lists", "Extracts AMK lists template to a certain folder and exits", cxxopts::value<fs::path>())
		("extract_driver", "Extracts SPC driver to a certain folder and exits", cxxopts::value<fs::path>())

		("h,help", "Print usage")
	;
	auto argp = options.parse(argc, argv);

	// Print usage if either this program is called with no arguments or with the -h option.
	if (argc == 1 || argp.count("help"))
    {
		std::cout << options.help() << std::endl;
		exit(0);
    }

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

	// Parsing the custom driver folder, if any.
	if (argp.count("driver"))
	{
		fs::path driver_folder = argp["driver"].as<fs::path>();
		o.spc_options.useCustomSPCDriver = true;
		o.spc_options.customSPCDriverPath = driver_folder;
		std::cerr << "Using custom SPC driver at " << fs::absolute(driver_folder).string() << std::endl;
	}
	
	return 0;
}