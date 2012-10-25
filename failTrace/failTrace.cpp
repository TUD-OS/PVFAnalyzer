/**********************************************************************
          DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.

**********************************************************************/

#include <iostream>	          // std::cout
#include <getopt.h>	          // getopt()
#include <boost/foreach.hpp>  // FOREACH
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/graph/graphviz.hpp>
#include "instruction/cfg.h"
#include "util.h"

struct FTConfig : public Configuration
{
	std::string input_filename;
	std::string trace_filename;
	std::string output_filename;

	FTConfig()
		: Configuration(), input_filename("a.out"),
	      output_filename("output.ilist")
	{ }
};

static FTConfig config;


static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-t <file>] [-o <file>] [-v] [-d]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set (ELF) input file [a.out]" << std::endl;
	std::cout << "\t-t <file>          Set FAIL/* input trace file" << std::endl;
	std::cout << "\t-o <file>          Write the resulting output to file. [output.ilist]" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
}


static void
banner()
{
	Version version = Configuration::get()->globalProgramVersion;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        Fail/* trace reader version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "df:ho:t:v")) != -1) {

		if (config.parse_option(opt))
			continue;

		switch(opt) {

			case 'f':
				config.input_filename = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return false;

			case 'o':
				config.output_filename = optarg;
				break;

			case 't':
				break;
		}
	}
	return true;
}


int main(int argc, char **argv)
{
	std::vector<Instruction*> iList;
	std::vector<InputReader*> inputs;

	Configuration::setConfig(&config);

	if (not parseInputFromOptions(argc, argv))
		exit(2);

	banner();

	FileInputReader *fr = new FileInputReader();
	inputs.push_back(fr);
	std::cout << "Reading ELF file: " << config.input_filename << std::endl;
	fr->addData(config.input_filename.c_str());

	return 0;
}

#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
