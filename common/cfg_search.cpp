#include "instruction/cfg.h"
#include "util.h"

#include <boost/graph/depth_first_search.hpp>


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

	void discover_vertex(CFGVertexDescriptor v, const ControlFlowGraph& g) const
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


CFGVertexDescriptor const
findCFGNodeWithAddress(ControlFlowGraph const &cfg, Address a, CFGVertexDescriptor startSearch)
{
	DEBUG(std::cout << "FIND(" << a.v << ") -> ";);
	try {
		BBForAddressFinder vis(a);
		boost::depth_first_search(cfg, boost::visitor(vis).root_vertex(startSearch));
	} catch (CFGVertexFoundException cvfe) {
		DEBUG(std::cout << cvfe._vd << std::endl;);
		return cvfe._vd;
	}

	DEBUG(std::cout << "[]" << std::endl;);
	throw NodeNotFoundException();
}