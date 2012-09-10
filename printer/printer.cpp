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
#include <iomanip>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/tuple/tuple.hpp>

#include "util.h"
#include "instruction/cfg.h"
#include "instruction/instruction_udis86.h"


struct PrinterConfiguration : public Configuration
{
	std::string input_filename;
	std::string output_filename;
	std::string color;
	bool quiet;

	PrinterConfiguration()
		: Configuration(), input_filename("input.cfg"),
	      output_filename("output.dot"),
	      color("simple"), quiet(false)
	{ }
};

static PrinterConfiguration conf;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-c  <strat>]] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-f <file>          Read CFG from file. [input.cfg]" << std::endl;
	std::cout << "\t-o <file>          Write graphviz output to file. [output.dot]" << std::endl;
	std::cout << "\t                   (use -o - to print to stdout)" << std::endl;
	std::cout << "\t-c [<strat>]       Colorize output graph using one of the following" << std::endl;
	std::cout << "\t                   strategies [simple]:" << std::endl;
	std::cout << "\t                      simple -> all in one color" << std::endl;
	std::cout << "\t                      call   -> call depths are colored the same" << std::endl;
	std::cout << "\t                      comp   -> strongly connected components (cycles) colored the same" << std::endl;
	std::cout << "\t                      func   -> colorize BBs belonging to the same function with the same color" << std::endl;
	std::cout << "\t-q                 quiet - generate a graph without code / address information [off]" << std::endl;
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

	while ((opt = getopt(argc, argv, "c::f:ho:qv")) != -1) {
		if (conf.parse_option(opt))
			continue;

		switch(opt) {
			case 'c':
				if (optarg) {
					if (optarg[0] == '=')
						optarg += 1;
					conf.color = optarg;
				}
				else {
					conf.color = "simple";
				}
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
			case 'q':
				conf.quiet = true;
				break;
		}
	}
	return true;
}


void readCFG(ControlFlowGraph& cfg)
{
	try {
		cfg.fromFile(conf.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


class GraphColoringStrategy
{
protected:
	ControlFlowGraph const & _g;
public:
	virtual int selectColorIndex(CFGVertexDescriptor const &v, int maxColors) = 0;

	GraphColoringStrategy(ControlFlowGraph const &g)
		: _g(g)
	{ }

	virtual ~GraphColoringStrategy() { }
};


class FunctionColoringStrategy : public GraphColoringStrategy
{
	std::map<CFGVertexDescriptor, int> functionColors;
	int nextColor;

public:
	FunctionColoringStrategy(const ControlFlowGraph& g)
		: GraphColoringStrategy(g), functionColors(),
		  nextColor(1)
	{ }

	virtual int selectColorIndex(CFGVertexDescriptor const& v, int max)
	{
		CFGNodeInfo const & node = _g.node(v);
		CFGVertexDescriptor dom = node.functionEntry;
		if (functionColors[dom] == 0) {
			functionColors[dom] = nextColor;
			nextColor++;
			if (nextColor >= max) nextColor = 1;
		}
		return functionColors[dom];
	}
};


class CallDepthColoringStrategy : public GraphColoringStrategy
{
	std::map<CFGVertexDescriptor, int>     _callDepth;
	std::map<Address, CFGVertexDescriptor> _callDominators;
public:
	CallDepthColoringStrategy(ControlFlowGraph const &g)
		: GraphColoringStrategy(g), _callDepth(), _callDominators()
	{ }

	virtual int selectColorIndex(CFGVertexDescriptor const& v, int max)
	{
		int depth = _callDepth[v];

		boost::graph_traits<ControlFlowGraphLayout>::out_edge_iterator edge, edgeEnd;

		for (boost::tie(edge, edgeEnd) = boost::out_edges(v, _g.cfg);
		     edge != edgeEnd; ++edge) {
			CFGVertexDescriptor t = boost::target(*edge, _g.cfg);
			if ((_g.node(v).bb->branchType == Instruction::BT_CALL) or
			    (_g.node(v).bb->branchType == Instruction::BT_CALL_DYN) or
			    (_g.node(v).bb->branchType == Instruction::BT_CALL_RESOLVE)) {
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
				Instruction* caller  = _g.node(v).bb->instructions.back();
				Address ret          = caller->ip() + caller->length();
				_callDominators[ret] = v;
			} else if (_g.node(v).bb->branchType == Instruction::BT_RET) {
				Address retTarget = _g.node(t).bb->firstInstruction();
				_callDepth[t] = _callDepth[_callDominators[retTarget]];
			} else {
				_callDepth[t] = depth;
			}
		}
		return depth;
	}
};


class StrongComponentColoringStrategy : public GraphColoringStrategy
{
	std::vector<int> component, discoverTime;
	std::vector<boost::default_color_type> color;
	std::vector<CFGVertexDescriptor> root;

public:
	StrongComponentColoringStrategy(ControlFlowGraph const & g)
		: GraphColoringStrategy(g), component(), discoverTime(), color(), root()
	{
		int vertexCount = boost::num_vertices(_g.cfg);
		component.reserve(vertexCount);
		discoverTime.reserve(vertexCount);
		color.reserve(vertexCount);
		root.reserve(vertexCount);
		boost::strong_components(_g.cfg, &component[0],
	                             boost::root_map(&root[0]).
	                             color_map(&color[0]).
	                             discover_time_map(&discoverTime[0]));
	}

	virtual int selectColorIndex(CFGVertexDescriptor const& v, int max)
	{
		return component[v] % max;
	}
};


class SingleColoringStrategy : public GraphColoringStrategy
{
public:
	SingleColoringStrategy(const ControlFlowGraph& g)
		: GraphColoringStrategy(g)
	{ }

	virtual int selectColorIndex(CFGVertexDescriptor const &, int)
	{
		return 0;
	}
};


class ColoringStrategyFactory
{
public:
	static GraphColoringStrategy* create(ControlFlowGraph const &g, PrinterConfiguration const& opt)
	{
		if (opt.color == "simple") {
			return new SingleColoringStrategy(g);
		} else if (opt.color == "call") {
			return new CallDepthColoringStrategy(g);
		} else if (opt.color == "comp") {
			return new StrongComponentColoringStrategy(g);
		} else if (opt.color == "func") {
			return new FunctionColoringStrategy(g);
		} else {
			std::cout << "Unknown coloring strategy '" << opt.color << "'. Using single color." << std::endl;
			return new SingleColoringStrategy(g);
		}
	}
};

/**
 * @brief Node writer for Graphviz CFG output
 **/
struct ExtendedGraphvizInstructionWriter
{
	ControlFlowGraph&          g;
	GraphColoringStrategy&     _strategy;
	std::map<int, std::string> _callDepthColors;
	int                        _maxCallDepthColor;

	ExtendedGraphvizInstructionWriter(ControlFlowGraph& _g, GraphColoringStrategy& strat)
		: g(_g), _strategy(strat), _callDepthColors(), _maxCallDepthColor(23)
	{
		_callDepthColors[ 0] = "brown2";
		_callDepthColors[ 1] = "dodgerblue";
		_callDepthColors[ 2] = "aquamarine";
		_callDepthColors[ 3] = "darkgoldenrod1";
		_callDepthColors[ 4] = "cornflowerblue";
		_callDepthColors[ 5] = "chocolate1";
		_callDepthColors[ 6] = "darkgreen";
		_callDepthColors[ 7] = "blue2";
		_callDepthColors[ 8] = "darkorchid";
		_callDepthColors[ 9] = "darkorange";
		_callDepthColors[10] = "crimson";
		_callDepthColors[11] = "darkolivegreen1";
		_callDepthColors[12] = "blueviolet";
		_callDepthColors[13] = "darkslateblue";
		_callDepthColors[14] = "gold";
		_callDepthColors[15] = "deeppink";
		_callDepthColors[16] = "forestgreen";
		_callDepthColors[17] = "cadetblue2";
		_callDepthColors[18] = "goldenrod2";
		_callDepthColors[19] = "hotpink";
		_callDepthColors[20] = "green2";
		_callDepthColors[21] = "cyan2";
		_callDepthColors[22] = "chartreuse";
		_callDepthColors[23] = "khaki";
	}

	void operator() (std::ostream& out, const CFGVertexDescriptor &v)
	{
		out << " [shape=box,fontname=Terminus,fontsize=8,style=filled,color=";
		out << _callDepthColors[_strategy.selectColorIndex(v, _maxCallDepthColor)] << ",";

		BasicBlock* bb = g.node(v).bb;
		if (!conf.quiet and !bb->instructions.empty()) {
			out << "label=\"(" << v << ") [@0x";
			out << std::hex << bb->firstInstruction().v << "]\\l";
			BOOST_FOREACH(Instruction* i, bb->instructions) {
				out << "0x" << std::setw(8) << std::hex << std::setfill('0')
				    << i->ip().v << ": " << i->c_str() << "\\l";
			}
			out << "\"";
		} else {
			out << "label=\"(" << v << ")\"";
		}
		out << "]";
		}
};


struct GraphvizEdgeWriter
{
	template <class Edge>
	void operator() (std::ostream& out, Edge const &e)
	{
		out << "[penwidth=2,arrowsize=1.5]";
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

	GraphColoringStrategy* colStrat = ColoringStrategyFactory::create(cfg, conf);
	ExtendedGraphvizInstructionWriter gnw(cfg, *colStrat);
	boost::write_graphviz(out, cfg.cfg, gnw, GraphvizEdgeWriter());

	delete colStrat;
	cfg.releaseNodeMemory();
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
