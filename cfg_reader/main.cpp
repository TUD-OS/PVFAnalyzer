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

/**
 * @brief Command line long options
 **/
struct option my_opts[] = {
	{"file",    required_argument, 0, 'f'},
	{"help",    no_argument,       0, 'h'},
	{"hex",     no_argument,       0, 'x'},
	{"outfile", required_argument, 0, 'o'},
	{0,0,0,0} // this line be last
};

static std::string output_filename = "output.cfg";

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-x <bytestream>] [-f <file>]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Parse binary file (ELF or raw binary)" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-x <bytes>         Interpret the following two-digit hexadecimal numbers" << std::endl;
	std::cout << "\t                   as input to work on." << std::endl;
	std::cout << "\t-o <file>          Write the resulting CFG to file." << std::endl;
}


static void
banner()
{
	struct {
		unsigned minor;
		unsigned major;
	} version =  {0,0};
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

	while ((opt = getopt(argc, argv, "f:ho:x")) != -1) {
		switch(opt) {
			case 'f': { // file input
					std::cout << "input file: " << argv[optind-1] << std::endl;
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
				std::cout << "OUT: " << optarg << std::endl;
				output_filename = optarg;
				break;
		}
	}
	return true;
}

/**
 * @brief CFG Node data
 **/
struct CFGNodeInfo
{
	Instruction* instruction; // right now we only store an instruction

	CFGNodeInfo(Instruction *inst = 0)
		: instruction(inst)
	{ }

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		a & instruction;
	}
};

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS,
                              boost::bidirectionalS,
                              CFGNodeInfo> ControlFlowGraph;


/**
 * @brief Node writer for Graphviz CFG output
 **/
struct GraphvizInstructionWriter
{
	ControlFlowGraph& g;

	GraphvizInstructionWriter(ControlFlowGraph& _g)
		: g(_g)
	{ }

	template <class Vertex>
	void operator() (std::ostream& out, const Vertex &v) const
	{
		out << " [shape=box,";
		if (g[v].instruction) {
			out << "label=\"\\[0x" << std::hex << std::setw(8)
			    << g[v].instruction->ip() << "\\]\\n"
			    << g[v].instruction->c_str() << "\"";
		} else {
			out << "label=\"<empty>\"";
		}
		out << "]";
	}
};

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

				i->print();
				std::cout << std::endl;
				ip   += i->length();
				offs += i->length();
				//delete i;
			}
		}
	}


	/* ---------- CFG Postprocessing ---------- */

	std::cout << "vertices in CFG: " << boost::num_vertices(cfg)
	          << " " << boost::num_edges(cfg) << std::endl;

	boost::write_graphviz(std::cout, cfg, GraphvizInstructionWriter(cfg));

	/* Store graph */
	std::ofstream ofs(output_filename);
	boost::archive::binary_oarchive oa(ofs);
	oa << cfg;

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

	std::cout << "Read " << count_bytes(input) << " bytes of input." << std::endl;
	std::cout << "input stream:\n";
	dump_sections(input);

	std::cout << "---------\n";
	buildCFG(input);

	cleanup(input);

	return 0;
}
