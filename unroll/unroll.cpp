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
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "instruction/cfg.h"
#include "util.h"

struct UnrollConfig : public Configuration
{
	std::string input_filename;
	std::string output_filename;
	std::vector<int> bbList;

	UnrollConfig()
		: Configuration(), input_filename("output.cfg"),
	      output_filename("output.ilist"), bbList()
	{ }
};

static UnrollConfig config;


static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-v] [-d]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set input file [output.cfg]" << std::endl;
	std::cout << "\t-o <file>          Write the resulting output to file. [output.ilist]" << std::endl;
	std::cout << "\t-b BB1,BB2,...     Comma-separated list of BBs to unrolll [empty]" << std::endl;
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
	std::cout << "\033[33m" << "        CFG Unroller version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static void
parseBBList(UnrollConfig& conf, char const *optarg)
{
	std::string s(optarg);
	boost::tokenizer<> tokens(s);
	for (boost::tokenizer<>::iterator it = tokens.begin();
	     it != tokens.end(); ++it) {
		try {
			int i = boost::lexical_cast<int>(*it);
			std::cout << "BB: " << i << std::endl;
			conf.bbList.push_back(i);
		} catch (boost::bad_lexical_cast) {
			std::cout << "Error casting '" << *it << "' to int. Skipping." << std::endl;
		}
	}
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "b:df:ho:v")) != -1) {

		if (config.parse_option(opt))
			continue;

		switch(opt) {
			case 'b':
				parseBBList(config, optarg);
				break;

			case 'f':
				config.input_filename = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return false;

			case 'o':
				config.output_filename = optarg;
				break;
		}
	}
	return true;
}


static void
readCFG(ControlFlowGraph& cfg)
{
	try {
		CFGFromFile(cfg, config.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}

int main(int argc, char **argv)
{
	Configuration::setConfig(&config);

	if (not parseInputFromOptions(argc, argv))
		exit(2);

	banner();

	ControlFlowGraph cfg;
	readCFG(cfg);

	std::vector<Instruction*> iList;
	BOOST_FOREACH(int node, config.bbList) {
		BOOST_FOREACH(Instruction *instr, cfg[node].bb->instructions) {
			if (config.debug) {
				instr->print();
				std::cout << "\n";
			}
			iList.push_back(instr);
		}
	}

	std::ofstream outFile(config.output_filename);
	if (!outFile.good()) {
		std::cout << "Could not open file '" << config.output_filename << "'" << std::endl;
		return 1;
	}
	boost::archive::binary_oarchive oa(outFile);
	oa << iList;
	outFile.close();
	std::cout << "Written to " << config.output_filename << std::endl;

	return 0;
}

#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop