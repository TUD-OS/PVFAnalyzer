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

// vi:ft=cpp
#include "instruction/basicblock.h"
#include "instruction/cfg.h"
#include "instruction/disassembler.h"
#include "util.h"

#include <boost/foreach.hpp>
#include <boost/graph/depth_first_search.hpp>

/*
 * Data that is returned from building a single
 * new basic block.
 */
struct BBInfo {
	BasicBlock          *bb;      // BB pointer
	std::vector<Address> targets; // list of targets this BB branches to

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
	Udis86Disassembler dis; ///> underlying disassembler
	ControlFlowGraph   _cfg; ///> control flow graph

	/*
	 * List of input buffers to read instructions from.
	 * The builder algorithm ignores all branch targets
	 * that do not reside within these buffers.
	 */
	std::vector<InputReader*> const &inputs;

	/**
	 * @brief Explore a single basic block
	 */
	BBInfo exploreSingleBB(Address e);

	/**
	 * @brief Find input buffer containing an address
	 */
	RelocatedMemRegion bufferForAddress(Address a)
	{
		//DEBUG(std::cerr << __func__ << "(" << a << ")" << std::endl;);
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
		: dis(), _cfg(), inputs(in)
	{ }

	virtual void build(Address a);

	virtual ControlFlowGraph const& cfg() { return _cfg; }
};

/**
 * @brief CFG Builder singleton accessor 
 */
CFGBuilder* CFGBuilder::get(std::vector<InputReader*> const& in)
{
	static CFGBuilder_priv p(in);
	return &p;
}


/* Q: Using BGL, how to stop a {depth_first|breadth_first|*}_search once you're
 *    done without visiting all other useless nodes?
 * A: Throw an exception!
 *
 * http://stackoverflow.com/questions/1500709/how-do-i-stop-the-breadth-first-search-using-boost-graph-library-when-using-a-cu
 */
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
			DEBUG(std::cout << "Vertex " << v << " "
	                         << bb->firstInstruction() << "-" << bb->lastInstruction()
	                         << " srch " << _a << std::endl;);
			if ((_a >= bb->firstInstruction()) and (_a <= bb->lastInstruction()))
				throw CFGVertexFoundException(v);
		}
	}
};

/* We will find a lot of initially unresolved links by exploring a new
 * basic block and finding its terminating instruction, which points to
 * one or more other addresses. This type represents dangling links.
 */
typedef std::pair<CFGVertexDescriptor, Address> UnresolvedLink;

struct AddressComparator
{
	BasicBlock *_b;
	AddressComparator(BasicBlock *b) : _b(b) { }

	bool operator() (UnresolvedLink u)
	{
		return ((_b->firstInstruction() <= u.second) and (u.second <= _b->lastInstruction()));
	}
};


void CFGBuilder_priv::build(Address entry)
{
	DEBUG(std::cout << __func__ << "(" << std::hex << entry << ")" << std::endl;);

	/* This list stores all yet unresolved links. */;
	std::list<UnresolvedLink> bb_connections;

	/* Create an initial CFG node */
	CFGVertexDescriptor initialVD = boost::add_vertex(CFGNodeInfo(new BasicBlock()), _cfg);

	/* The init node links directly to entry point. This is the first link we explore
	 * in the loop below. */
	bb_connections.push_back(UnresolvedLink(initialVD, entry));

	/* As long as we have unresolved connections... */
	while (!bb_connections.empty()) {

		UnresolvedLink next = bb_connections.front();
		bb_connections.pop_front();
		DEBUG(std::cout << "Exploring next BB starting @ " << (void*)next.second << std::endl;);

		BBInfo bbi = exploreSingleBB(next.second);
		DEBUG(bbi.dump(); std::cout << std::endl; );

		if (bbi.bb != 0) {
			DEBUG(std::cout << bbi.bb << std::endl;);
			/* We definitely found a _new_ vertex here. So add it to the CFG. */
			CFGVertexDescriptor vd = boost::add_vertex(CFGNodeInfo(bbi.bb), _cfg);

			DEBUG(std::cout << "Adding incoming edges..." << std::endl;);
			/* We need to add an edge from the previous vd */
			boost::add_edge(next.first, vd, _cfg);
			/* Also, there might be more BBs with this target BB in the queue. */
			std::list<UnresolvedLink>::iterator n =
				std::find_if(bb_connections.begin(), bb_connections.end(), AddressComparator(bbi.bb));
			while (n != bb_connections.end()) {
				BasicBlock* b = _cfg[(*n).first].bb;
				/* If the unresolved link goes to our start address, add an edge, ... */
				if (b->firstInstruction() == bbi.bb->firstInstruction()) {
					boost::add_edge((*n).first, vd, _cfg);
				} else { /* ... otherwise, split right now.*/
					throw NotImplementedException("split BB");
				}
				bb_connections.erase(n);
				n = std::find_if(bb_connections.begin(), bb_connections.end(), AddressComparator(bbi.bb));
			}

			DEBUG(std::cout << "Scanning for BBs with outgoing targets." << std::endl;);
			/* Now establish target links */
			BOOST_FOREACH(Address a, bbi.targets) {
				/*
				 * Is the target within an alreay known BB? The result is one
				 * of three possible outcomes:
				 *
				 * 1) Yes, and the target BB's start address is the target.
				 *     -> add a vertex between the BBs
				 * 2) Yes, and the target BB's start address is _not_ the target.
				 *     -> jump into a middle of a BB -> these are actually
				 *        two BBs -> split the BBs
				 * 3) No
				 *     -> Add an unresolved link and discover later.
				 */
				try {
					BBForAddressFinder vis(a);
					boost::depth_first_search(_cfg, boost::visitor(vis));
				} catch (CFGVertexFoundException cvfe) {
					DEBUG(std::cout << "Found a basic block: " << cvfe._vd << std::endl;);
					/* Case 1 */
					if (a == _cfg[cvfe._vd].bb->firstInstruction()) {
						DEBUG(std::cout << "jump goes to beginning of BB. Adding CFG vertex." << std::endl;);
						boost::add_edge(vd, cvfe._vd, _cfg);
					} else { /* case 2 */
						DEBUG(std::cout << "need to split BB" << std::endl;);
						throw NotImplementedException("basic block split");
					}
					continue;
				}

				DEBUG(std::cout << "This is no BB I know about yet. Queuing " << a << " for discovery." << std::endl;);
				// case 3: need to discover more code first
				bb_connections.push_back(UnresolvedLink(vd, a));
			}
		} else {
			// XXX: Actually, we'd insert dummy targets here. The most likely
			//      use case are code snippets referencing code that is external
			//      from the input buffers' view.
			std::cout << "An error happened while parsing a BB?" << std::endl;
			throw NotImplementedException(__func__);
		}
	}
	DEBUG(std::cout << "\033[32m" << "BB construction finished." << "\033[0m" << std::endl;);
}


BBInfo CFGBuilder_priv::exploreSingleBB(Address e)
{
	BBInfo bbi;
	RelocatedMemRegion buf = bufferForAddress(e);

	if (buf.size == 0) { // no buffer found
		DEBUG(std::cout << "Did not find code for entry point " << (void*)e << std::endl;);
		return bbi;
	}

	bbi.bb         = new BasicBlock();
	unsigned offs  = e - buf.mapped_base;
	Instruction *i;

	dis.buffer(buf);
	do {
		i = dis.disassemble(offs);

		if (i) {
			DEBUG(i->print(); std::cout << std::endl;);
			bbi.bb->add_instruction(i);
			offs += i->length();

			if (i->isBranch()) {
				DEBUG(std::cout << "Found branch. BB terminates here." << std::endl;);
				Address target = i->branchTarget();
				bbi.targets.push_back(target);
				// XXX: might have more than one branch target!
				//throw NotImplementedException("Branch target detection");
				break;
			}

		} else { // end of input
			DEBUG(std::cout << "End of input. BB terminates here." << std::endl;);
		}

	} while (i);

	if (bbi.bb->instructions.size() == 0) {
		DEBUG(std::cout << "No instructions in BB?" << std::endl;);
		delete bbi.bb;
		bbi.bb = 0;
	}

	return bbi;
}
