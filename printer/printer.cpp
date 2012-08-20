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

#include <getopt.h>
#include <iostream>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/tuple/tuple.hpp>

#include "util.h"
#include "instruction/cfg.h"
#include "instruction/instruction_udis86.h"


struct PrinterConfiguration : public Configuration
{
	std::string input_filename;
	std::string output_filename;
	bool color;

	PrinterConfiguration()
		: Configuration(), input_filename("input.cfg"),
	      output_filename("output.dot"),
	      color(false)
	{ }
};

static PrinterConfiguration conf;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-c] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-f <file>          Read CFG from file. [input.cfg]" << std::endl;
	std::cout << "\t-o <file>          Write graphviz output to file. [output.dot]" << std::endl;
	std::cout << "\t                   (use -o - to print to stdout)" << std::endl;
	std::cout << "\t-c                 Colorize output graph [off]" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
}


static void
banner()
{
	Version version = conf.globalProgramVersion;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Printer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "cf:ho:v")) != -1) {
		if (conf.parse_option(opt))
			continue;

		switch(opt) {
			case 'c':
				conf.color = true;
				break;
			case 'f':
				conf.input_filename = optarg;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'o':
				conf.output_filename = optarg;
				break;
		}
	}
	return true;
}


void readCFG(ControlFlowGraph& cfg)
{
	try {
		CFGFromFile(cfg, conf.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}

/**
 * @brief Node writer for Graphviz CFG output
 **/
struct ExtendedGraphvizInstructionWriter
{
	ControlFlowGraph& g;
	std::map<CFGVertexDescriptor, int> _callDepth;
	std::map<int, std::string>         _callDepthColors;
	std::map<Address, CFGVertexDescriptor> _callDominators;
	int                                _maxCallDepthColor;

	ExtendedGraphvizInstructionWriter(ControlFlowGraph& _g)
		: g(_g), _callDepth(), _callDepthColors(), _callDominators(), _maxCallDepthColor(10)
	{
		_callDepthColors[ 0] = "steelblue";
		_callDepthColors[ 1] = "crimson";
		_callDepthColors[ 2] = "orange2";
		_callDepthColors[ 3] = "palegreen2";
		_callDepthColors[ 4] = "lightsalmon";
		_callDepthColors[ 5] = "palevioletred3";
		_callDepthColors[ 6] = "royalblue";
		_callDepthColors[ 7] = "peachpuff1";
		_callDepthColors[ 8] = "orchid2";
		_callDepthColors[ 9] = "forestgreen";
		_callDepthColors[10] = "brown1";
	}

	std::string callDepthColor(CFGVertexDescriptor const & v)
	{
		int depth = _callDepth[v];

		boost::graph_traits<ControlFlowGraph>::out_edge_iterator edge, edgeEnd;

		for (boost::tie(edge, edgeEnd) = boost::out_edges(v, g);
		     edge != edgeEnd; ++edge) {
			CFGVertexDescriptor t = boost::target(*edge, g);
			if (g[v].bb->branchType == Instruction::BT_CALL) {
				/*
				 * The target of a caller is colored one level
				 * deeper than the current level.
				 */
				if (_callDepth[t] == 0) {
					_callDepth[t]    = depth+1;
				}

				/*
				 * To properly color all ret edges, we need to be aware of
				 * which call dominates a RET edge.
				 */
				Instruction* caller  = g[v].bb->instructions.back();
				Address ret          = caller->ip() + caller->length();
				_callDominators[ret] = v;
			} else if (g[v].bb->branchType == Instruction::BT_RET) {
				Address retTarget = g[t].bb->firstInstruction();
				_callDepth[t] = _callDepth[_callDominators[retTarget]];
			} else {
				_callDepth[t] = depth;
			}
		}

		return _callDepthColors[depth];
	}

	void operator() (std::ostream& out, const CFGVertexDescriptor &v)
	{
		out << " [shape=box,fontname=Terminus,";
		if (conf.color) {
			std::string color = callDepthColor(v);
			out << "style=filled,color=" << color << ",";
		}
		BasicBlock* bb = g[v].bb;
		if (!bb->instructions.empty()) {
			out << "label=\"[@0x";
			out << std::hex << bb->firstInstruction() << "]\\l";
			BOOST_FOREACH(Instruction* i, bb->instructions) {
				out << i->c_str() << "\\l";
			}
			out << "\"";
		} else {
			out << "label=\"<empty>\"";
		}
		out << "]";
		}
};

void writeCFG(ControlFlowGraph& cfg)
{
	std::streambuf *buf;  // output buffer pointer
	std::ofstream ofs;    // file stream if outfile != stdout

	if (conf.output_filename == "-") { // special output file '-'
		buf = std::cout.rdbuf();
	} else {
		ofs.open(conf.output_filename);
		if (!ofs.is_open()) {
			std::cerr << "Could not open '" << conf.output_filename
			          << "' for output. Redirecting to stdout."
			          << std::endl;
			buf = std::cout.rdbuf();
		} else {
			buf = ofs.rdbuf();
		}
	}

	std::ostream out(buf); // the real output stream

	ExtendedGraphvizInstructionWriter gnw(cfg);
	boost::write_graphviz(out, cfg, gnw);

	freeCFGNodes(cfg);
}


void work()
{
	ControlFlowGraph cfg;
	readCFG(cfg);
	writeCFG(cfg);
}


int main(int argc, char **argv)
{
	if (not parseInputFromOptions(argc, argv))
		return 1;

	Configuration::setConfig(&conf);

	banner();
	work();

	return 0;
}
#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
