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
#include <set>

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
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
		: _dis(), _cfg(cfg), _bbfound(), _inputs(in), _returnLocations(), _bb_connections()
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
	Udis86Disassembler  _dis;    ///> underlying disassembler
	ControlFlowGraph&  _cfg;     ///> control flow graph
	std::set<Address>  _bbfound; ///> start addresses of discovered BBs

	/*
	 * List of input buffers to read instructions from.
	 * The builder algorithm ignores all branch targets
	 * that do not reside within these buffers.
	 */
	std::vector<InputReader*> const &_inputs;
	/*
	 * For each basic block we keep track of its RET location, which is defined as:
	 *     RET(initial)  := NONE
	 *     RET(calleeBB) := callSites[callerBB.lastInstruction()]
	 *     RET(otherBB)  := RET(parentBB)
	 */
	ReturnLocationMap     _returnLocations;

	/* This list stores all yet unresolved links. We work until this
	 * list becomes empty. */;
	PendingResolutionList _bb_connections;

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
		BOOST_FOREACH(InputReader *ir, _inputs) {
			for (unsigned sec = 0; sec < ir->sectionCount(); ++sec) {
				RelocatedMemRegion mr = ir->section(sec)->getBuffer();
				if (mr.relocContains(a))
					return mr;
			}
		}
		return RelocatedMemRegion();
	}

	void addCFGEdge(CFGVertexDescriptor start, CFGVertexDescriptor target)
	{
		DEBUG(std::cout << "\033[34m[add_edge]\033[0m " << start << " -> " << target << std::endl;);
		boost::add_edge(start, target, _cfg);
	}


	void dumpUnresolvedLinks()
	{
		std::cout << "\033[35mBB_CONN:\033[0m [ ";
		BOOST_FOREACH(UnresolvedLink l, _bb_connections) {
			std::cout << "(" << l.first << ", " << l.second << ") ";
		}
		std::cout << "]" << std::endl;
	}

	void updatePendingList(CFGVertexDescriptor oldVert, CFGVertexDescriptor newVert)
	{
		BOOST_FOREACH(UnresolvedLink& l, _bb_connections) {
			if (l.first == oldVert) {
				DEBUG(std::cout << l.first << " ~> " << newVert << std::endl;);
				l.first = newVert;
			}
		}
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
	void handleOutgoingEdges(BBInfo& bbi, CFGVertexDescriptor newVertex);

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
		if (Configuration::get()->debug) {
			if (prevBB->instructions.empty()) {
				std::cout << "XX ";
			} else {
				std::cout << prevBB->firstInstruction();
			}
			std::cout << " " << prevBB->branchType << std::endl;
		}

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
		DEBUG(std::cout << "returnLocations[" << bbi.bb->firstInstruction() << "] := " << returnLocations[bbi.bb] << std::endl;);

		/*
		 * If the BB is a RETurn, we need to add another target, which the disassembler
		 * cannot tell us about, because its not encoded in the instruction.
		 */
		if (bbi.bb->branchType == Instruction::BT_RET) {
			Address retloc = returnLocations[bbi.bb];
			DEBUG(std::cout << "RET to " << std::hex << retloc << std::endl;);
			if (retloc != ~0UL)
				bbi.targets.push_back(retloc);
		}
	}

	CFGVertexDescriptor splitBasicBlock(CFGVertexDescriptor splitVertex, Address splitAddress);
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
			DEBUG(std::cout << "Vertex " << v << " "
	                         << bb->firstInstruction() << "-" << bb->lastInstruction()
	                         << " srch " << _a << std::endl;);
			if ((_a >= bb->firstInstruction()) and (_a <= bb->lastInstruction()))
				throw CFGVertexFoundException(v);
		}
	}
};
#pragma GCC diagnostic pop


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


struct AddressInBBComparator
{
	BasicBlock *_b;
	AddressInBBComparator(BasicBlock *b) : _b(b) { }

	bool operator() (CFGBuilder_priv::UnresolvedLink const & u) const
	{
		return ((_b->firstInstruction() <= u.second) and (u.second <= _b->lastInstruction()));
	}
};


struct InstructionAddressComparator
{
	Address _a;
	InstructionAddressComparator(Address a) : _a(a) {}

	bool operator() (Instruction* inst) const
	{
		return inst->ip() == _a;
	}
};


void CFGBuilder_priv::build(Address entry)
{
	DEBUG(std::cout << __func__ << "(" << std::hex << entry << ")" << std::endl;);

	/* CALL/RET discovery is tricky: we first discover a CALL site, but don't know
	 * the BB that returns from that call yet. We therefore keep track of 2 maps:
	 *
	 * 1) For each call site, we store a mapping from the location of the CALL to
	 *    the location the callee is going to return to.
	 */
	CallSiteMap           callSites;

	/* Create an initial CFG node */
	CFGVertexDescriptor initialVD = boost::add_vertex(CFGNodeInfo(new BasicBlock()), _cfg);

	/* No BB to return to from init node */
	_returnLocations[_cfg[initialVD].bb] = ~0UL;

	/* The init node links directly to entry point. This is the first link we explore
	 * in the loop below. */
	_bb_connections.push_back(UnresolvedLink(initialVD, entry));

	/* As long as we have unresolved connections... */
	while (!_bb_connections.empty()) {

		DEBUG(dumpUnresolvedLinks(););
		UnresolvedLink next = _bb_connections.front();
		_bb_connections.pop_front();
		DEBUG(std::cout << "\033[36m-0- Exploring next BB starting @ " << (void*)next.second
		                << "\033[0m" << std::endl;);

		BBInfo bbi = exploreSingleBB(next.second);
		DEBUG(bbi.dump(); std::cout << std::endl; );

		if (bbi.bb != 0) {
			DEBUG(std::cout << "Basic block @ " << bbi.bb << std::endl;);
			DEBUG(std::cout << "--1-- Updating call sites" << std::endl;);
			updateCallsites(bbi, callSites);

			DEBUG(std::cout << "--2-- Updating return locations" << std::endl;);
			BasicBlock* prevBB = _cfg[next.first].bb;
			updateReturnLocations(bbi, prevBB, _returnLocations, callSites);

			/* We definitely found a _new_ vertex here. So add it to the CFG. */
			CFGVertexDescriptor vd = boost::add_vertex(CFGNodeInfo(bbi.bb), _cfg);

			DEBUG(std::cout << "--3-- Handling incoming edges" << std::endl;);
			handleIncomingEdges(next.first, vd, bbi);
			DEBUG(std::cout << "--4-- Handling outgoing edges" << std::endl;);
			handleOutgoingEdges(bbi, vd);
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

	_dis.buffer(buf);
	/* We disassemble instructions as long as we find some in the buffer ... */
	do {
		i = _dis.disassemble(offs);

		if (i) {
			DEBUG(i->print(); std::cout << std::endl;);
			bbi.bb->addInstruction(i);
			offs += i->length();

			if (i->isBranch()) { // .. except, we find a branch instruction
				DEBUG(std::cout << "Found branch. BB terminates here." << std::endl;);
				bbi.bb->branchType = i->branchTargets(bbi.targets);
				break;
			}

			if (_bbfound.find(i->ip()) != _bbfound.end()) { // .. or we run into the start of an
			                                                // already discovered BB
				DEBUG(std::cout << "Start of already known BB. Terminating discovery." << std::endl;);
				bbi.targets.push_back(i->ip()); // store connection

				/* remove and delete duplicate instruction right now */
				bbi.bb->instructions.pop_back(); // we know this was the last inserted instruction
				delete i;

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

	/* cache BB start address */
	_bbfound.insert(bbi.bb->firstInstruction());

	DEBUG(std::cout << "Finished. BB instructions: " << bbi.bb->firstInstruction() << " - " << bbi.bb->lastInstruction() << std::endl;);
	return bbi;
}


void CFGBuilder_priv::handleIncomingEdges(CFGVertexDescriptor prevVertex,
                                          CFGVertexDescriptor newVertex,
                                          BBInfo& bbi)
{
		DEBUG(std::cout << "Adding incoming edges to " << bbi.bb->firstInstruction()
		                << "..." << std::endl;);

		/* We need to add an edge from the previous vd */
		addCFGEdge(prevVertex, newVertex);

		PendingResolutionList::iterator n =
			std::find_if(_bb_connections.begin(), _bb_connections.end(), AddressInBBComparator(bbi.bb));
		while (n != _bb_connections.end()) {
			BasicBlock *b = _cfg[(*n).first].bb;
			DEBUG(std::cout << "Unresolved link: " << b->firstInstruction() << " -> "
			                << (*n).second << std::endl;);
			/* If the unresolved link goes to our start address, add an edge, ... */
			if ((*n).second == bbi.bb->firstInstruction()) {
				addCFGEdge((*n).first, newVertex);
			} else { /* ... otherwise, split right now.*/
				throw NotImplementedException("split BB");
			}
			_bb_connections.erase(n);
			n = std::find_if(_bb_connections.begin(), _bb_connections.end(), AddressInBBComparator(bbi.bb));
		}
}


void CFGBuilder_priv::handleOutgoingEdges(BBInfo& bbi, CFGVertexDescriptor newVertex)
{
	DEBUG(std::cout << "Adding outgoing targets." << std::endl;);
	if (Configuration::get()->debug) {
		std::cout << "   [";
		BOOST_FOREACH(Address a, bbi.targets) {
			std::cout << a << ",";
		}
		std::cout << "]" << std::endl;
	}

	/* Now establish target links */
	while (!bbi.targets.empty()) {
		Address a = bbi.targets.front();
		bbi.targets.erase(bbi.targets.begin());
		DEBUG(std::cout << "Next outgoing target address: " << a << std::endl;);

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
		CFGVertexDescriptor targetNode;
		try {
			targetNode = findCFGNodeWithAddress(a);
		} catch (NodeNotFoundException) {
			DEBUG(std::cout << "This is no BB I know about yet. Queuing 0x" << a << " for discovery." << std::endl;);
			// case 3: need to discover more code first
			_bb_connections.push_back(UnresolvedLink(newVertex, a));
			continue;
		}

		if (a == _cfg[targetNode].bb->firstInstruction()) {
			DEBUG(std::cout << "jump goes to beginning of BB. Adding CFG edge." << std::endl;);
			addCFGEdge(newVertex, targetNode);
		} else {
			DEBUG(std::cout << "need to split BB " << targetNode << std::endl;);
			CFGVertexDescriptor splitTailVertex = splitBasicBlock(targetNode, a);

			/* We now require special handling in case we split the very BB we
			 * are currently working on. In this case, we need to continue updating
			 * outgoing targets, but use the newly created BB with the remaining
			 * out targets.
			 *
			 * If we split a _different_ BB, simply continue.
			 */
			if (targetNode == newVertex) {
				addCFGEdge(splitTailVertex, splitTailVertex);

				BBInfo bbi2  = bbi;
				bbi2.bb      = _cfg[splitTailVertex].bb;
				bbi2.targets = bbi.targets;
				bbi.targets.clear(); // orig BB is done
				handleOutgoingEdges(bbi2, splitTailVertex);
			} else {
				addCFGEdge(newVertex, splitTailVertex);
			}

			/*
			 * Adjust pending link list. All non-discovered links that started from
			 * the non-split block now start from the newly created vertex.
			 */
			updatePendingList(targetNode, splitTailVertex);
		}
	}
}


CFGVertexDescriptor CFGBuilder_priv::splitBasicBlock(CFGVertexDescriptor splitVertex, Address splitAddress)
{
	// 1) create new empty BB
	BasicBlock* bb2       = new BasicBlock();
	CFGNodeInfo& bbNode   = _cfg[splitVertex];

	/*
	 * Branch type: new BB inherits the old one's.
	 * Old BB becomes a JUMP_UNCOND node.
	 */
	bb2->branchType       = bbNode.bb->branchType;
	bbNode.bb->branchType = Instruction::BT_JUMP_UNCOND;

	/*
	 * Set return location. This is simply the same as for
	 * the old BB.
	 */
	_returnLocations[bb2]  = _returnLocations[bbNode.bb];

	DEBUG(std::cout << "split: new BB @ " << bb2 << std::endl;);

	/* Find the first instruction of the new BB. */
	std::vector<Instruction*>::iterator iit = std::find_if(bbNode.bb->instructions.begin(),
	                                                       bbNode.bb->instructions.end(),
	                                                       InstructionAddressComparator(splitAddress));
	/*
	 * We came here because someone deemed us to split the BB. Hence, we must
	 * assume that the split instruction is within the BB!
	 */
	assert(iit != bbNode.bb->instructions.end());

	/*
	 * Now move the new BB's instructions over to the new container.
	 */
	while (iit != bbNode.bb->instructions.end()) {
		DEBUG(std::cout << "split: moving instr. @ " << (*iit)->ip()
		                << " to bb2" << std::endl;);
		bb2->instructions.push_back(*iit);
		/*
		 * C++ trivia: to be sure that your iterator is valid after
		 *             erasing from a container, you need to obtain
		 *             it as erase()'s return value.
		 */
		iit = bbNode.bb->instructions.erase(iit);
	}

	/* Ready to add new vertex to the CFG. */
	CFGVertexDescriptor vert2 = boost::add_vertex(CFGNodeInfo(bb2), _cfg);

	// 3) incoming edges for original BB remain untouched

	/*
	 * 4) all existing outgoing edges from bb1 are transformed to
	 *    outgoing edges of bb2
	 */
	boost::graph_traits<ControlFlowGraph>::out_edge_iterator edge0, edgeEnd;
	for (boost::tie(edge0, edgeEnd) = boost::out_edges(splitVertex, _cfg);
			edge0 != edgeEnd; ++edge0) {
		CFGVertexDescriptor targetV = boost::target(*edge0, _cfg);
		addCFGEdge(vert2, targetV);
		DEBUG(std::cout << "split: removing: " << splitVertex << " -> " << targetV << std::endl;);
		boost::remove_edge(splitVertex, targetV, _cfg);
	}

	// 5) add edge from bb1 -> bb2
	addCFGEdge(splitVertex, vert2);

	return vert2;
}