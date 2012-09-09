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

#include "instruction/cfg.h"
#include "util.h"

#include <boost/graph/depth_first_search.hpp>
#include <boost/tuple/tuple.hpp>

#include <queue>
#include <functional>

/* Q: Using BGL, how to stop a {depth_first|breadth_first|*}_search once you're
 *    done without visiting all other useless nodes?
 * A: Throw an exception!
 *
 * http://stackoverflow.com/questions/1500709/how-do-i-stop-the-breadth-first-search-using-boost-graph-library-when-using-a-cu
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
struct CFGVertexFoundException
{
	CFGVertexDescriptor _vd;
	CFGVertexFoundException(CFGVertexDescriptor v)
		: _vd(v)
	{ }
};


/**
 * @brief CFG node visitor to find BB containing an address
 */
class BBForAddressFinder : public boost::default_dfs_visitor {
	Address _a;
public:
	BBForAddressFinder(Address a)
		: _a(a)
	{}

	void discover_vertex(CFGVertexDescriptor v, const ControlFlowGraphLayout& g) const
	{
		BasicBlock *bb = g[v].bb;

		// skip empty basic blocks (which only appear in the initial BB)
		if (bb->instructions.size() > 0) {
#if 0
			DEBUG(std::cout << "Vertex " << v << " "
	                         << bb->firstInstruction().v << "-" << bb->lastInstruction().v
	                         << " srch " << _a.v << std::endl;);
#endif
			if ((_a >= bb->firstInstruction()) and (_a <= bb->lastInstruction()))
				throw CFGVertexFoundException(v);
		}
	}
};
#pragma GCC diagnostic pop

enum class BFSColor : unsigned char
{
	WHITE,
	GRAY,
	BLACK,
};

void privateBFSTraversal(ControlFlowGraph cfg, CFGVertexDescriptor start,
                         std::function<void(CFGVertexDescriptor)> functor)
{
	std::queue<CFGVertexDescriptor> Q;
	BFSColor colorMap[boost::num_vertices(cfg.cfg)];

	for (unsigned idx = 0; idx < boost::num_vertices(cfg.cfg); ++idx) {
		colorMap[idx] = BFSColor::WHITE;
	}

	colorMap[start] = BFSColor::GRAY;
	Q.push(start);

	while (!Q.empty()) {
		CFGVertexDescriptor vert = Q.front(); Q.pop(); functor(vert);
		boost::graph_traits<ControlFlowGraphLayout>::out_edge_iterator out, out_end;
		boost::graph_traits<ControlFlowGraphLayout>::in_edge_iterator  in,  in_end;

		for (boost::tie(out, out_end) = boost::out_edges(vert, cfg.cfg); out != out_end; ++out) {
			CFGVertexDescriptor vd = boost::target(*out, cfg.cfg);
			BFSColor color         = colorMap[vd];
			if (color == BFSColor::WHITE) {
				colorMap[vd] = BFSColor::GRAY;
				Q.push(vd);
			}
		}

		for (boost::tie(in, in_end) = boost::in_edges(vert, cfg.cfg); in != in_end; ++in) {
			CFGVertexDescriptor vd = boost::source(*in, cfg.cfg);
			BFSColor color         = colorMap[vd];
			if (color == BFSColor::WHITE) {
				colorMap[vd] = BFSColor::GRAY;
				Q.push(vd);
			}
		}

		colorMap[vert] = BFSColor::BLACK;
	}
}


CFGVertexDescriptor const
findCFGNodeWithAddress(ControlFlowGraph const &cfg, Address a, CFGVertexDescriptor startSearch)
{
	DEBUG(std::cout << "FIND(" << a.v << ") -> " << std::endl;);
	try {
#if 1
		BBForAddressFinder vis(a);
		boost::depth_first_search(cfg.cfg, boost::visitor(vis));
#else
		auto fn = [&] (CFGVertexDescriptor vd) {
			DEBUG(std::cout << __func__ << vd << std::endl;);
			BasicBlock* bb = cfg[vd].bb;
			if ((vd > 0) and (bb->firstInstruction() <= a) and (a <= bb->lastInstruction())) {
				throw CFGVertexFoundException(vd);
			}
		};
		privateBFSTraversal(cfg, startSearch, fn);
#endif
	} catch (CFGVertexFoundException cvfe) {
		DEBUG(std::cout << cvfe._vd << std::endl;);
		return cvfe._vd;
	}

	DEBUG(std::cout << std::endl << "    []" << std::endl;);
	throw NodeNotFoundException();
}