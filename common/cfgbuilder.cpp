// vi:ft=cpp
#include "instruction/basicblock.h"
#include "instruction/cfg.h"
#include "instruction/disassembler.h"
#include "version.h"

#include <boost/foreach.hpp>
#include <boost/graph/depth_first_search.hpp>

struct BBInfo {
	BasicBlock *bb;
	std::vector<Address> targets;

	BBInfo()
		: bb(0), targets()
	{ }

	BBInfo(BBInfo const &b)
		: bb(b.bb), targets(b.targets)
	{ }

	BBInfo& operator=(BBInfo& other)
	{ return *this; }

public:
	void dump()
	{
		std::cout << "BB @ " << (void*)bb << " -> [";
		BOOST_FOREACH(Address a, targets) {
			if (a != *targets.begin())
				std::cout << ", ";
			std::cout << a;
		}
		std::cout << "]";
	}
};

class CFGBuilder_priv : public CFGBuilder
{
	Udis86Disassembler dis;
	ControlFlowGraph   cfg;

	std::vector<InputReader*> const &inputs;

	BBInfo generate(Address e);

	RelocatedMemRegion bufferForAddress(Address a)
	{
		DEBUG(std::cerr << __func__ << "(" << a << ")" << std::endl;);
		BOOST_FOREACH(InputReader *ir, inputs) {
			for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
				RelocatedMemRegion mr = ir->section(sec)->getBuffer();
				if (mr.reloc_contains(a))
					return mr;
			}
		}
		return RelocatedMemRegion();
	}

public:
	CFGBuilder_priv(std::vector<InputReader*> const& in)
		: dis(), cfg(), inputs(in)
	{ }

	virtual void build(Address a);
};

CFGBuilder* CFGBuilder::get(std::vector<InputReader*> const& in)
{
	static CFGBuilder_priv p(in);
	return &p;
}


/*
 * Basic block construction
 *
 * 1) Start with an empty BB
 * 2) Iterate over input until
 *    a) the input buffer is empty -> BB ends here, no new BBs discovered
 *    b) a branch instruction is hit
 *       -> BB ends here
 *       -> new BBs are to be discovered:
 *          a) JMP/CALL instruction:
 *             - jump target _and_ instruction following the last one become
 *               new BB beginnings
 *             - connections to those two new BBs are added to CFG
 *          b) RET: no new BB
 *             - connection to return address is added
 *          c) SYSENTER etc.: no new BB, no connection (XXX: but maybe special mark?)
 * 3) JMP targets may go into the middle of an already discovered BB
 *    -> split the BB into two, add respective connections
 */

struct BranchToAddress
{
	Address a;
	BranchToAddress(Address _a) : a(_a) { }

	bool operator () (std::pair<CFGVertexDescriptor, Address>& p)
	{
		return p.second == a;
	}
};

struct CFGVertexFoundException
{
	CFGVertexDescriptor _vd;
	CFGVertexFoundException(CFGVertexDescriptor v)
		: _vd(v)
	{ }
};


class dfs_visitor : public boost::default_dfs_visitor {
	Address _a;
public:
	dfs_visitor(Address a)
		: _a(a)
	{}

	/* XXX: We actually don't need a template fn, but only one instance of it */
	template <typename VERT, typename GRAPH>
	void discover_vertex(VERT v, const GRAPH& g) const
	{
		/* Q: How to stop a search once you're done?
		 * A: Throw an exception!
		 *
		 * http://stackoverflow.com/questions/1500709/how-do-i-stop-the-breadth-first-search-using-boost-graph-library-when-using-a-cu
		 */
		DEBUG(std::cout << v << std::endl;);
		CFGNodeInfo i = g[v];
		std::cout << i.bb << " ";
		if (i.bb) std::cout << i.bb->instructions.size();
		else std::cout << "[empty]";
		std::cout << std::endl;
	}
};

void CFGBuilder_priv::build (Address entry)
{
	DEBUG(std::cout << __func__ << "(" << std::hex << entry << ")" << std::endl;);

	typedef std::pair<CFGVertexDescriptor, Address> BBConn;
	std::list<BBConn> bb_connections;

	/* Create an initial CFG node */
	CFGVertexDescriptor initialVD = boost::add_vertex(CFGNodeInfo(new BasicBlock()), cfg);
	/* init node links directly to entry point */
	bb_connections.push_back(BBConn(initialVD, entry));

	/* As long as we have unresolved connections... */
	while (!bb_connections.empty()) {

		BBConn next = bb_connections.front();
		bb_connections.pop_front();

		BBInfo bbi = generate(entry);
		DEBUG(bbi.dump(); std::cout << std::endl; );

		if (bbi.bb != 0) {
			/*
			* Definitely a new CFG vertex
			*/
			std::cout << bbi.bb << std::endl;
			CFGVertexDescriptor vd = boost::add_vertex(CFGNodeInfo(bbi.bb), cfg);

			dfs_visitor vis(0);
			boost::depth_first_search(cfg, boost::visitor(vis));

			/* for each target of this BB: */
			BOOST_FOREACH(Address a, bbi.targets) {
				// already have a BB with this start address?
				// have a BB, but address points into its middle?
				dfs_visitor vis(a);
				boost::depth_first_search(cfg, boost::visitor(vis));

				// need to discover more code first
				bb_connections.push_back(BBConn(vd, a));
			}
		}
	}
}


BBInfo CFGBuilder_priv::generate(Address e)
{
	BBInfo bbi;
	bbi.bb                 = new BasicBlock();
	RelocatedMemRegion buf = bufferForAddress(e);

	if (buf.size == 0) {
		delete bbi.bb;
		bbi.bb = 0;
		return bbi;
	}

	unsigned offs  = e - buf.mapped_base;
	Instruction *i = 0;

	dis.buffer(buf);
	do {
		i = dis.disassemble(offs);

		if (i == 0) { /* End of input reached */
			DEBUG(std::cout << "End of input" << std::endl; );
		} else {
			DEBUG(i->print(); std::cout << std::endl;);
			bbi.bb->add_instruction(i);
			offs += i->length();
		}

	} while (i);

	if (bbi.bb->instructions.size() == 0) {
		delete bbi.bb;
		bbi.bb = 0;
	}

	return bbi;
}
