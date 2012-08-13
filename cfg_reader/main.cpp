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
#include <boost/tuple/tuple.hpp>
#include "data/input.h"            // DataSection, InputReader
#include "instruction/disassembler.h"
#include "instruction/instruction.h"
#include "instruction/cfg.h"
#include "util.h"

/**
 * @brief Command line long options
 **/
struct option my_opts[] = {
	{"debug",   no_argument,       0, 'd'},
	{"entry",   required_argument, 0, 'e'},
	{"file",    required_argument, 0, 'f'},
	{"help",    no_argument,       0, 'h'},
	{"hex",     no_argument,       0, 'x'},
	{"outfile", required_argument, 0, 'o'},
	{"verbose", no_argument,       0, 'v'},
	{0,0,0,0} // this line be last
};

struct ReaderConfig : public Configuration
{
	std::string output_filename;
	Address entryPoint;

	ReaderConfig()
		: Configuration(), output_filename("output.cfg"), entryPoint(0)
	{ }
};

static ReaderConfig conf;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-x <bytestream>] [-f <file>] [-o <file>] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Parse binary file (ELF or raw binary)" << std::endl;
	std::cout << "\t-x <bytes>         Interpret the following two-digit hexadecimal" << std::endl;
	std::cout << "\t                   numbers as input to work on." << std::endl;
	std::cout << "\t-o <file>          Write the resulting CFG to file. [output.cfg]" << std::endl;
	std::cout << "\t-e <addr>          Set decoding entry point address [0x00000000]" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
}


static void
banner()
{
	version_t version = Configuration::get()->global_program_version;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Analyzer version " << version.major()
	          << "." << version.minor() << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv, std::vector<InputReader*>& retvec)
{
	int opt;

	while ((opt = getopt(argc, argv, "e:df:ho:xv")) != -1) {

		if (conf.parse_option(opt))
			continue;

		switch(opt) {
			case 'f': { // file input
					FileInputReader *fr = new FileInputReader();
					retvec.push_back(fr);
					fr->addData(argv[optind-1]);
			}
			break;

			case 'x': { // hex dump input
					int idx = optind;
					HexbyteInputReader *reader = new HexbyteInputReader();
					while (idx < argc) {
						if (argv[idx][0] == '-') { // next option found
							optind = idx;
							break;
						} else {
							reader->addData(argv[idx]);
						}
						++idx;
					}
					retvec.push_back(reader);
				}
				break;

			case 'h':
				usage(argv[0]);
				return false;

			case 'o':
				conf.output_filename = optarg;
				break;

			case 'e':
				conf.entryPoint = strtoul(optarg, 0, 0);
				break;
		}
	}
	return true;
}

static void
buildCFG(std::vector<InputReader*> const & v)
{
	ControlFlowGraph cfg;
	CFGBuilder* builder = CFGBuilder::get(v, cfg);
	DEBUG(std::cout << "Builder @ " << (void*)builder << std::endl;);
	try {
		VERBOSE(std::cout << "Decoding. Entry point 0x" << std::hex << conf.entryPoint << std::endl;);
		builder->build(conf.entryPoint);
	} catch (NotImplementedException e) {
		std::cout << "\033[31;1mNot implemented:\033[0m " << e.message << std::endl;
	}

	/* Store graph */
	CFGToFile(cfg, conf.output_filename);
	std::cout << "Wrote CFG to '" << conf.output_filename << "'" << std::endl;

	/*
	 * Cleanup: we need to delete the instructions in the
	 * CFG's vertex nodes.
	 */
	freeCFGNodes(cfg);
}


static unsigned
count_bytes(std::vector<InputReader*> const & rv)
{
	unsigned bytes = 0;
	BOOST_FOREACH(InputReader* ir, rv) {
		for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
			bytes += ir->section(sec)->bytes();
		}
	}

	return bytes;
}


static void
dump_sections(std::vector<InputReader*> const & rv)
{
	BOOST_FOREACH(InputReader* ir, rv) {
	for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
			ir->section(sec)->dump();
		}
	}
}


static void
cleanup(std::vector<InputReader*> &rv)
{
	while (!rv.empty()) {
		std::vector<InputReader*>::iterator i = rv.begin();
		delete *i;
		rv.erase(i);
	}
}

int
main(int argc, char **argv)
{
	using namespace std;

	std::vector<InputReader*> input;

	if (not parseInputFromOptions(argc, argv, input))
		exit(2);

	Configuration::setConfig(&conf);

	banner();

	if (input.size() == 0)
		exit(1);

	if (conf.verbose) std::cout << "Read " << count_bytes(input) << " bytes of input." << std::endl;
	if (conf.verbose) {
		std::cout << "input stream:\n";
		dump_sections(input);
		std::cout << "---------\n";
	}

	buildCFG(input);

	cleanup(input);

	return 0;
}
#include "instruction/instruction_udis86.h"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");