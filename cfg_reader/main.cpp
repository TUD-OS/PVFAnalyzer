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
#include "input.h"            // DataSection, InputReader
#include "disassembler.h"
#include "instruction.h"
#include "cfg.h"
#include "version.h"

/**
 * @brief Command line long options
 **/
struct option my_opts[] = {
	{"file",    required_argument, 0, 'f'},
	{"help",    no_argument,       0, 'h'},
	{"hex",     no_argument,       0, 'x'},
	{"outfile", required_argument, 0, 'o'},
	{"verbose", no_argument,       0, 'v'},
	{0,0,0,0} // this line be last
};

static bool verbose                = false;
static std::string output_filename = "output.cfg";

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-x <bytestream>] [-f <file>] [-o <file>] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Parse binary file (ELF or raw binary)" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-x <bytes>         Interpret the following two-digit hexadecimal" << std::endl;
	std::cout << "\t                   numbers as input to work on." << std::endl;
	std::cout << "\t-o <file>          Write the resulting CFG to file. [output.cfg]" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
}


static void
banner()
{
	version_t version = global_program_version;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Analyzer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv, std::vector<InputReader*>& retvec)
{
	int opt;

	while ((opt = getopt(argc, argv, "f:ho:xv")) != -1) {
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
						//std::cout << optind << " " << argv[idx] << endl;
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
				output_filename = optarg;
				break;

			case 'v':
				verbose = true;
				break;
		}
	}
	return true;
}

static void
buildCFG(std::vector<InputReader*> const & v)
{
	Udis86Disassembler dis;
	ControlFlowGraph   cfg;

	// create initial dummy node (start)
	boost::graph_traits<ControlFlowGraph>::vertex_descriptor lastDesc
		= boost::add_vertex(CFGNodeInfo(0), cfg);
	boost::graph_traits<ControlFlowGraph>::vertex_descriptor nextDesc;

	BOOST_FOREACH(InputReader* ir, v) {
		for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
			dis.buffer(ir->section(sec)->getBuffer());

			Address ip     = dis.buffer().mapped_base;
			unsigned offs  = 0;
			Instruction* i = 0;

			while ((i = dis.disassemble(offs)) != 0) {

				nextDesc   = boost::add_vertex(CFGNodeInfo(i), cfg);
				boost::add_edge(lastDesc, nextDesc, cfg);
				/* XXX: need to add other targets here XXX */
				lastDesc   = nextDesc; // sequential...

				if (verbose) {
					i->print();
					std::cout << std::endl;
				}
				ip   += i->length();
				offs += i->length();
				//delete i;
			}
		}
	}

	/* Store graph */
	std::ofstream ofs(output_filename);
	boost::archive::binary_oarchive oa(ofs);
	oa << cfg;
	std::cout << "Wrote CFG to '" << output_filename << "'" << std::endl;

	/*
	 * Cleanup: we need to delete the instructions in the
	 * CFG's vertex nodes.
	 */
	boost::graph_traits<ControlFlowGraph>::vertex_iterator vi, vi_end;
	for (boost::tie(vi, vi_end) = boost::vertices(cfg);
		 vi != vi_end; ++vi) {
		if (cfg[*vi].instruction) {
			delete cfg[*vi].instruction;
		}
	}
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

	banner();

	if (input.size() == 0)
		exit(1);

	if (verbose) std::cout << "Read " << count_bytes(input) << " bytes of input." << std::endl;
	if (verbose) {
		std::cout << "input stream:\n";
		dump_sections(input);
		std::cout << "---------\n";
	}

	buildCFG(input);

	cleanup(input);

	return 0;
}
