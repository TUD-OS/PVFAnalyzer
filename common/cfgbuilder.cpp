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

#include <map>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/graph/depth_first_search.hpp>

/**
 * @brief Internal BB info used by CFGBuilder_priv
 *
 * In addition to the BB itself, this data also incorporates connection
 * information, which later is stored as edges inside the CFG.
 **/
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
	/**
	 * @brief Dump BBInfo
	 *
	 * @return void
	 **/
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


/**
 * @brief Actual implementation of the CFG builder
 **/
class CFGBuilder_priv : public CFGBuilder
{
public:
	CFGBuilder_priv(std::vector<InputReader*> const& in, ControlFlowGraph& cfg)
		: dis(), _cfg(cfg), inputs(in)
	{ }

	virtual void build(Address a);

	/*
	 * We will find a lot of initially unresolved links by exploring a new
	 * basic block and finding its terminating instruction, which points to
	 * one or more other addresses. This type represents dangling links.
	 */
	typedef std::pair<CFGVertexDescriptor, Address> UnresolvedLink;

	typedef std::list<UnresolvedLink>               PendingResolutionList;
	typedef std::map<Address, Address>              CallSiteMap;
	typedef std::map<BasicBlock*, Address>          ReturnLocationMap;

private:
	Udis86Disassembler  dis; ///> underlying disassembler
	ControlFlowGraph&  _cfg; ///> control flow graph

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
	 * @brief Find the CFG node containing a given EIP
	 *
	 * @param a EIP
	 * @return CFGVertexDescriptor
	 **/
	CFGVertexDescriptor const findCFGNodeWithAddress(Address a);

	/**
	 * @brief Find input buffer containing an address
	 */
	RelocatedMemRegion bufferForAddress(Address a)
	{
		//DEBUG(std::cerr << __func__ << "(" << a << ")" << std::endl;);
		BOOST_FOREACH(InputReader *ir, inputs) {
			for (unsigned sec = 0; sec < ir->sectionCount(); ++sec) {
				RelocatedMemRegion mr = ir->section(sec)->getBuffer();
				if (mr.relocContains(a))
					return mr;
			}
		}
		return RelocatedMemRegion();
	}

	/**
	 * @brief Helper: handle incoming edges for newly discovered BB
	 *
	 * When discovering a new BB, we need to go through the list of
	 * pending resolutions to check if there are other unresolved links
	 * to the newly discovered basic block. If so, we either a) add an
	 * additional edge or b) need to split the BB.
	 *
	 * @param prevVertex vertex we started discovering this BB from
	 * @param newVertex  new vertex
	 * @param pending    list of pending connections
	 * @param bbi        BBInfo descriptor for new BB
	 * @return void
	 **/
	void handleIncomingEdges(CFGVertexDescriptor prevVertex,
	                         CFGVertexDescriptor newVertex,
	                         PendingResolutionList& pending,
	                         BBInfo& bbi);

	/**
	 * @brief Helper: handle outgoing edges for a newly discovered BB
	 *
	 * Adds outgoing edges for the new BB. If the target address is in
	 * an already known BB, we add an outgoing edge. If we don't know
	 * about the target BB yet, we add a new unresolved link.
	 *
	 * @param bbi       Basic Block Info
	 * @param newVertex new vertex
	 * @param pending   list of pending resolutions
	 * @return void
	 **/
	void handleOutgoingEdges(BBInfo& bbi, CFGVertexDescriptor newVertex,
	                         PendingResolutionList& pending);

	/**
	 * @brief Helper: update call sites with new BB
	 *
	 * To properly identify the targets of RET instructions, we need
	 * to keep a list of call sites and their successor instructions during
	 * BB discovery. This list needs to be updated for every newly discovered
	 * BB that is terminated by a CALL.
	 *
	 * @param bbi   new Basic Block
	 * @param calls list of call sites during BB discovery
	 * @return void
	 **/
	void updateCallsites(BBInfo& bbi, CallSiteMap& calls)
	{
		/* For each call, store the potential location of a later RET */
		if (bbi.bb->branchType == Instruction::BT_CALL) {
			Instruction *lastInst = bbi.bb->instructions.back();
			DEBUG(std::cout << "storing call site: " << std::hex << bbi.bb->lastInstruction()
			                << " -> " << lastInst->ip() + lastInst->length() << std::endl;);
			calls[bbi.bb->lastInstruction()] = lastInst->ip() + lastInst->length();
		}
	}

	/**
	 * @brief Helper: update return locations
	 *
	 * When encountering a RET instruction, we need to determine where this
	 * BB is returning to. To do so, each BB is assigned a RET location. When
	 * coming from a CALL BB, this RET location is updated to the CALL BB's successor
	 * instruction (see updateCallsites()). For every other BB, the RET location is
	 * identical to the RET location of its parent BB.
	 *
	 * @param bbi             new basic block
	 * @param prevBB          parent BB
	 * @param returnLocations list of potential return sites
	 * @param callSites       list of call sites
	 * @return void
	 **/
	void updateReturnLocations(BBInfo& bbi, BasicBlock* prevBB,
	                           ReturnLocationMap& returnLocations,
	                           CallSiteMap& callSites)
	{
		/* 
		 * If the _previous_ branch was a CALL, this subgraph will have a
		 * new return location set.
		 */
		DEBUG(std::cout << prevBB << " " << prevBB->branchType << std::endl;);

		if (prevBB->branchType == Instruction::BT_CALL) {
			returnLocations[bbi.bb] = callSites[prevBB->lastInstruction()];
		} else if (prevBB->branchType == Instruction::BT_RET) {
			Address retloc = returnLocations[bbi.bb];
			DEBUG(std::cout << std::dec << __LINE__ << ": retloc " << std::hex << retloc << std::endl;);
			if (retloc) {
				CFGVertexDescriptor const node = findCFGNodeWithAddress(returnLocations[bbi.bb]);
				returnLocations[bbi.bb] = returnLocations[_cfg[node].bb];
			} else
				returnLocations[bbi.bb] = ~0;
		} else {
			returnLocations[bbi.bb] = returnLocations[prevBB];
		}

		/*
		 * If the BB is a RETurn, we need to add another target, which the disassembler
		 * cannot tell us about, because its not encoded in the instruction.
		 */
		if (bbi.bb->branchType == Instruction::BT_RET) {
			Address retloc = returnLocations[bbi.bb];
			DEBUG(std::cout << "RET to " << std::hex << retloc << std::endl;);
			if (retloc != ~0)
				bbi.targets.push_back(retloc);
		}
	}
};

/**
 * @brief CFG Builder singleton accessor 
 */
CFGBuilder* CFGBuilder::get(std::vector<InputReader*> const& in, ControlFlowGraph& cfg)
{
	static CFGBuilder_priv p(in, cfg); // XXX might want to have more than one?
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


CFGVertexDescriptor const CFGBuilder_priv::findCFGNodeWithAddress(Address a)
{
	try {
		BBForAddressFinder vis(a);
		boost::depth_first_search(_cfg, boost::visitor(vis));
	} catch (CFGVertexFoundException cvfe) {
		return cvfe._vd;
	}

	throw NodeNotFoundException();
}


struct AddressComparator
{
	BasicBlock *_b;
	AddressComparator(BasicBlock *b) : _b(b) { }

	bool operator() (CFGBuilder_priv::UnresolvedLink const & u) const
	{
		return ((_b->firstInstruction() <= u.second) and (u.second <= _b->lastInstruction()));
	}
};


void CFGBuilder_priv::build(Address entry)
{
	DEBUG(std::cout << __func__ << "(" << std::hex << entry << ")" << std::endl;);

	/* This list stores all yet unresolved links. We work until this
	 * list becomes empty. */;
	PendingResolutionList bb_connections;

	/* CALL/RET discovery is tricky: we first discover a CALL site, but don't know
	 * the BB that returns from that call yet. We therefore keep track of 2 maps:
	 *
	 * 1) For each call site, we store a mapping from the location of the CALL to
	 *    the location the callee is going to return to.
	 */
	CallSiteMap           callSites;
	/*
	 * 2) For each basic block we keep track of its RET location, which is defined as:
	 *     RET(initial)  := NONE
	 *     RET(calleeBB) := callSites[callerBB.lastInstruction()]
	 *     RET(otherBB)  := RET(parentBB)
	 */
	ReturnLocationMap     returnLocations;

	/* Create an initial CFG node */
	CFGVertexDescriptor initialVD = boost::add_vertex(CFGNodeInfo(new BasicBlock()), _cfg);

	/* No BB to return to from init node */
	returnLocations[_cfg[initialVD].bb] = ~0UL;

	/* The init node links directly to entry point. This is the first link we explore
	 * in the loop below. */
	bb_connections.push_back(UnresolvedLink(initialVD, entry));

	/* As long as we have unresolved connections... */
	while (!bb_connections.empty()) {

		UnresolvedLink next = bb_connections.front();
		bb_connections.pop_front();
		DEBUG(std::cout << "\033[36m-0- Exploring next BB starting @ " << (void*)next.second
		                << "\033[0m" << std::endl;);

		BBInfo bbi = exploreSingleBB(next.second);
		DEBUG(bbi.dump(); std::cout << std::endl; );

		if (bbi.bb != 0) {
			DEBUG(std::cout << "--1-- Updating call sites" << std::endl;);
			updateCallsites(bbi, callSites);

			DEBUG(std::cout << "--2-- Updating return locations" << std::endl;);
			BasicBlock* prevBB = _cfg[next.first].bb;
			updateReturnLocations(bbi, prevBB, returnLocations, callSites);

			DEBUG(std::cout << bbi.bb << std::endl;);
			/* We definitely found a _new_ vertex here. So add it to the CFG. */
			CFGVertexDescriptor vd = boost::add_vertex(CFGNodeInfo(bbi.bb), _cfg);

			DEBUG(std::cout << "--3-- Handling incoming edges" << std::endl;);
			handleIncomingEdges(next.first, vd, bb_connections, bbi);
			DEBUG(std::cout << "--4-- Handling outgoing edges" << std::endl;);
			handleOutgoingEdges(bbi, vd, bb_connections);
			DEBUG(std::cout << "--5-- BB finished" << std::endl;);
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
	unsigned offs  = e - buf.mappedBase;
	Instruction *i;

	dis.buffer(buf);
	/* We disassemble instructions as long as we find some in the buffer ... */
	do {
		i = dis.disassemble(offs);

		if (i) {
			DEBUG(i->print(); std::cout << std::endl;);
			bbi.bb->addInstruction(i);
			offs += i->length();

			if (i->isBranch()) { // .. except, we find a branch instruction
				DEBUG(std::cout << "Found branch. BB terminates here." << std::endl;);
				bbi.bb->branchType = i->branchTargets(bbi.targets);
				break;
			}

		} else { // end of input
			DEBUG(std::cout << "End of input. BB terminates here." << std::endl;);
			break;
		}
	} while (i);

	/*
	 * Bogus: we found a BB with 0 instructions -> delete it right now.
	 *
	 * XXX: Should this ever happen?
	 */
	if (bbi.bb->instructions.size() == 0) {
		DEBUG(std::cout << "No instructions in BB?" << std::endl;);
		delete bbi.bb;
		bbi.bb = 0;
	}

	DEBUG(std::cout << "Finished. BB instructions: " << bbi.bb->firstInstruction() << " - " << bbi.bb->lastInstruction() << std::endl;);
	return bbi;
}


void CFGBuilder_priv::handleIncomingEdges(CFGVertexDescriptor prevVertex,
                                          CFGVertexDescriptor newVertex,
                                          PendingResolutionList& pending,
                                          BBInfo& bbi)
{
		DEBUG(std::cout << "Adding incoming edges..." << std::endl;);

		/* We need to add an edge from the previous vd */
		boost::add_edge(prevVertex, newVertex, _cfg);

		PendingResolutionList::iterator n =
			std::find_if(pending.begin(), pending.end(), AddressComparator(bbi.bb));
		while (n != pending.end()) {
			BasicBlock *b = _cfg[(*n).first].bb;
			/* If the unresolved link goes to our start address, add an edge, ... */
			if (b->firstInstruction() == bbi.bb->firstInstruction()) {
				boost::add_edge((*n).first, newVertex, _cfg);
			} else { /* ... otherwise, split right now.*/
				throw NotImplementedException("split BB");
			}
			pending.erase(n);
			n = std::find_if(pending.begin(), pending.end(), AddressComparator(bbi.bb));
		}
}


void CFGBuilder_priv::handleOutgoingEdges(BBInfo& bbi, CFGVertexDescriptor newVertex,
                                          PendingResolutionList& pending)
{
	DEBUG(std::cout << "Adding outgoing targets." << std::endl;);
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
			CFGVertexDescriptor const node = findCFGNodeWithAddress(a);
			if (a == _cfg[node].bb->firstInstruction()) {
				DEBUG(std::cout << "jump goes to beginning of BB. Adding CFG vertex." << std::endl;);
				boost::add_edge(newVertex, node, _cfg);
			} else {
				DEBUG(std::cout << "need to split BB" << std::endl;);
				throw NotImplementedException("basic block split");
			}
		} catch (NodeNotFoundException) {
			DEBUG(std::cout << "This is no BB I know about yet. Queuing 0x" << a << " for discovery." << std::endl;);
			// case 3: need to discover more code first
			pending.push_back(UnresolvedLink(newVertex, a));
		}
	}
}
