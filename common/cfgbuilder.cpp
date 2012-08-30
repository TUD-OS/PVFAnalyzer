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
			std::cout << a.v;
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
		: _dis(), _cfg(cfg), _bbfound(), _inputs(in), _callDoms(),
		  _callRets(), _retDoms(), _bb_connections()
	{ }

	virtual void build(Address a);

	/*
	 * We will find a lot of initially unresolved links by exploring a new
	 * basic block and finding its terminating instruction, which points to
	 * one or more other addresses. This type represents dangling links.
	 */
	typedef std::pair<CFGVertexDescriptor, Address> UnresolvedLink;
	typedef std::list<UnresolvedLink>               PendingResolutionList;

	typedef std::map<Address, CFGVertexDescriptor>  CallDominatorInfo;
	typedef std::map<Address, std::set<Address> >   CallReturnSites;
	typedef std::map<Address, std::set<CFGVertexDescriptor> > ReturnDominatorInfo;

private:

	CFGNodeInfo& cfg(CFGVertexDescriptor const vd) { return _cfg[vd]; }

	Udis86Disassembler  _dis;    ///> underlying disassembler
	ControlFlowGraph&  _cfg;     ///> control flow graph
	std::set<Address>  _bbfound; ///> start addresses of discovered BBs (used for tracking already discovered BBs if we enter them)

	/*
	 * List of input buffers to read instructions from.
	 * The builder algorithm ignores all branch targets
	 * that do not reside within these buffers.
	 */
	std::vector<InputReader*> const &_inputs;

	/*
	 * CALL/RET handling is a mess. :(
	 *
	 * A When we encounter a CALL, we know to which address we will return (if
	 *   the program is well-behaving!), but not, from which BB we will return,
	 *   because the called function itself may consist of multiple BBs.
	 * B There may be multiple calls to a function and thus also multiple
	 *   return edges.
	 * C When we encounter a RET, we know that we need to return to a previous
	 *   call site, but we might not know anymore, which site this was.
	 * D A function subgraph may be constructed only partially, which means some
	 *   RET BBs have already been added to the CFG, while others only exist as
	 *   pending links.
	 *
	 * - To solve problems A and B, we store a list of locations a called subgraph
	 *   eventually returns to along with the first vertex in this subgraph. This
	 *   vertex is the target of the CALL and dominates all other vertices in the
	 *   subgraph. We call it the *CALL DOMINATOR*.
	 * - To solve problem C, we attribute a call dominator to every BB in the CFG.
	 *   When encountering a RET, we can then use the dominator's return list to
	 *   add the appropriate return edges. Call dominators are assigned as follows:
	 *   * The target of a CALL instruction dominates itself.
	 *   * The target of a RET instruction is dominated by the dominator of the
	 *     BB the corresponding CALL came from. (Functions nest and we only need
	 *     to know about one layer of CALL/RET.)
	 *   * All other BBs inherit the dominator of their parent BB.
	 * - Due to problem D, both, the callDom as well as the callRets data structures,
	 *   use a mapping from a start address to a vertex / return list. Thereby, we
	 *   can store info about discovered and undiscovered return edges similarly.
	 */
	CallDominatorInfo   _callDoms;
	CallReturnSites     _callRets;
	ReturnDominatorInfo _retDoms;

	/* This list stores all yet unresolved links. We work until this
	 * list becomes empty. */;
	PendingResolutionList _bb_connections;

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
		BOOST_FOREACH(InputReader *ir, _inputs) {
			DataSection *sec;
			if ((sec = ir->sectionForAddress(a)) != 0) {
				return sec->getBuffer();
			}
		}
		return RelocatedMemRegion();
	}

	bool isDynamicBranch(Address a)
	{
		BOOST_FOREACH(InputReader *ir, _inputs) {
			if (ir->insideJumpTable(a))
				return true;
		}
		return false;
	}

	/**
	 * @brief Add an edge to the CFG
	 *
	 * @param start start vertex
	 * @param target end vertex
	 * @return void
	 **/
	void addCFGEdge(CFGVertexDescriptor start, CFGVertexDescriptor target)
	{
		DEBUG(std::cout << "\033[34m[add_edge]\033[0m " << start << " -> " << target << std::endl;);
		boost::add_edge(start, target, _cfg);
	}

	/**
	 * @brief Debugging: dump the list of currently unresolved links
	 *
	 * @return void
	 **/
	void dumpUnresolvedLinks()
	{
		std::cout << "\033[35mBB_CONN:\033[0m [ ";
		BOOST_FOREACH(UnresolvedLink l, _bb_connections) {
			std::cout << "(" << l.first << ", " << l.second.v << ") ";
		}
		std::cout << "]" << std::endl;
	}

	/**
	 * @brief Update list of unresolved links after a BB split
	 *
	 * After splitting a BB, all pending unresolved links from the original
	 * BB need to be updated to become pending unresolved links from the
	 * newly created tail BB.
	 *
	 * @param oldVert old BB
	 * @param newVert new split BB
	 * @return void
	 **/
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
	 * @return vertex descriptor -- by default the same as newVertex; if the vertex had
	 *                              to be split, returns the tail BB.
	 **/
	CFGVertexDescriptor handleIncomingEdges(CFGVertexDescriptor prevVertex,
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
	 * @brief Update CALL Dominator information
	 *
	 * @param parent node we came from
	 * @param newDesc new node
	 * @return void
	 **/
	void updateCallDoms(CFGVertexDescriptor parent, CFGVertexDescriptor newDesc)
	{
		DEBUG(std::cout << "\033[32;1m" << parent << " -> " << newDesc << "\033[0m" << std::endl;);
		CFGNodeInfo& parentNode       = cfg(parent);
		CFGNodeInfo& newNode          = cfg(newDesc);

		if (newNode.bb->branchType == Instruction::BT_CALL) {
			std::vector<Address> targets;
			Instruction *lastInst =newNode.bb->instructions.back();
			Address ret = lastInst->ip() + lastInst->length();
			DEBUG(std::cout << "   1._callDoms[" << ret.v
			                << "] := " << newDesc << std::endl;);
			_callDoms[ret] = newDesc;
		}

		/*
		 * We trick around a bit: a callee node not only sets its own dominator
		 * (to its own node), but also sets a dominator for the return from the
		 * respective caller. This is, because only here we know about which location
		 * that is.
		 */
		if (parent == 0) {
			_callDoms[newNode.bb->firstInstruction()] = newDesc;
		} else if (parentNode.bb->branchType == Instruction::BT_CALL) {
			DEBUG(std::cout << "   2._callDoms[" << newNode.bb->firstInstruction().v
			                << "] := " << newDesc << std::endl;);
			_callDoms[newNode.bb->firstInstruction()] = newDesc;
		} else {
			DEBUG(std::cout << "   3._callDoms[" << newNode.bb->firstInstruction().v
			                << "] := " << _callDoms[parentNode.bb->firstInstruction()] << std::endl;);
			_callDoms[newNode.bb->firstInstruction()] = _callDoms[parentNode.bb->firstInstruction()];
		}
	}


	/**
	 * @brief Given a CALL instruction, update the call target's list of return locations.
	 *
	 * @param parent parent node (e.g., the CALL)
	 * @param newDesc target node (the callee)
	 * @return void
	 **/
	void updateReturnsForCall(CFGVertexDescriptor parent, CFGVertexDescriptor newDesc)
	{
		CFGNodeInfo& parentNode = cfg(parent);
		CFGNodeInfo& newNode    = cfg(newDesc);

		if (parentNode.bb->branchType != Instruction::BT_CALL) {
			return;
		}

		Instruction* callingInstr = parentNode.bb->instructions.back();
		Address returnAddress     = callingInstr->ip() + callingInstr->length();
		Address startInstr        = newNode.bb->firstInstruction();
		DEBUG(std::cout << "callrets[" << startInstr.v << "] += [" << returnAddress.v << "]" << std::endl;);
		_callRets[startInstr].insert(returnAddress);
	}


	/**
	 * @brief Update the list of return dominators
	 *
	 * that is, the list of RET nodes belonging to a callee node.
	 *
	 * @param node node to insert (if it is RET)
	 * @return void
	 **/
	void updateRetDoms(CFGVertexDescriptor node)
	{
		CFGVertexDescriptor callDom = _callDoms[cfg(node).bb->firstInstruction()];
		if ((callDom != 0) and (cfg(node).bb->branchType == Instruction::BT_RET)) {
			Address a = cfg(callDom).bb->firstInstruction();
			_retDoms[a].insert(node);
		}
	}


	/**
	 * @brief For a given call, add the return edges.
	 *
	 * Given a CALL edge, update the callee's return edges. This
	 * may require two different steps
	 *   1) If the return target is already a CFG node, add an edge between
	 *      all the callee's RET nodes and the return target.
	 *   2) If the return target is not a vertex yet, add new unresolved
	 *      links from the RET nodes.
	 *
	 * @param caller caller CFG node
	 * @param callee callee CFG node
	 * @return void
	 **/
	void addReturnEdgesForCall(CFGVertexDescriptor caller, CFGVertexDescriptor callee)
	{
		DEBUG(std::cout << caller << " -> " << callee << std::endl;);

		Address target        = cfg(callee).bb->firstInstruction();
		Instruction *ret      = cfg(caller).bb->instructions.back();
		Address returnAddress = ret->ip() + Address(ret->length());

		/*
		 * For recursive functions we can encounter the case where we actually
		 * return to a BB that has already been discovered.
		 */
		CFGVertexDescriptor retVertex;
		bool haveVertex = false;
		try {
			retVertex  = findCFGNodeWithAddress(_cfg, returnAddress, caller);
			haveVertex = true;
		} catch (NodeNotFoundException)
		{ }

		DEBUG(std::cout << "ret doms [ " << target.v << "] :" << std::endl;);
		BOOST_FOREACH(CFGVertexDescriptor vd, _retDoms[target]) {
			DEBUG(std::cout << vd << std::endl;);
			if (haveVertex) {
				boost::add_edge(vd, retVertex, _cfg);
			} else {
				_bb_connections.push_back(UnresolvedLink(vd, returnAddress));
			}
		}
	}

	/**
	 * @brief Split a basic block at the given address
	 *
	 * @param splitVertex CFG node to split
	 * @param splitAddress address to split at
	 * @return CFGVertexDescriptor
	 **/
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


void CFGBuilder_priv::build(Address entry)
{
	DEBUG(std::cout << __func__ << "(" << std::hex << entry.v << ")" << std::endl;);

	/* Create an initial CFG node */
	CFGVertexDescriptor initialVD = boost::add_vertex(CFGNodeInfo(new BasicBlock()), _cfg);

	/* The init node links directly to entry point. This is the first link we explore
	 * in the loop below. */
	_bb_connections.push_back(UnresolvedLink(initialVD, entry));

	/* As long as we have unresolved connections... */
	while (!_bb_connections.empty()) {

		DEBUG(dumpUnresolvedLinks(););
		UnresolvedLink next = _bb_connections.front();
		_bb_connections.pop_front();
		DEBUG(std::cout << "\033[36m-0- Exploring next BB starting @ " << (void*)next.second.v
		                << "\033[0m" << std::endl;);

		BBInfo bbi = exploreSingleBB(next.second);
		DEBUG(bbi.dump(); std::cout << std::endl; );

		if (bbi.bb != 0) {
			DEBUG(std::cout << "Basic block @ " << bbi.bb << std::endl;);

			/* We definitely found a _new_ vertex here. So add it to the CFG. */
			CFGVertexDescriptor vd = boost::add_vertex(CFGNodeInfo(bbi.bb), _cfg);

			DEBUG(std::cout << "--1-- Handling incoming edges" << std::endl;);
			vd = handleIncomingEdges(next.first, vd, bbi);
			DEBUG(std::cout << "--2-- Handling outgoing edges" << std::endl;);
			handleOutgoingEdges(bbi, vd);
			DEBUG(std::cout << "--3-- BB finished" << std::endl;);
		} else {
			std::cout << "Ignoring empty BB (@ " << (void*)next.second.v << ")." << std::endl;
		}
	}
	DEBUG(std::cout << "\033[32m" << "BB construction finished." << "\033[0m" << std::endl;);
}


BBInfo CFGBuilder_priv::exploreSingleBB(Address e)
{
	BBInfo bbi;
	RelocatedMemRegion buf = bufferForAddress(e);

	if (buf.size == 0) { // no buffer found
		DEBUG(std::cout << "Did not find code for entry point " << (void*)e.v << std::endl;);
		return bbi;
	}

	bbi.bb         = new BasicBlock();
	Address offs   = e - buf.mappedBase;
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

	if (bbi.bb->instructions.size() == 0) {
		throw ThisShouldNeverHappenException("No instructions in BB");
	}

	/* cache BB start address */
	_bbfound.insert(bbi.bb->firstInstruction());

	/*
	 * Dynamic jump adjustment. For now we ignore any calls / jumps that go into
	 * dynamic loader's offset tables, because we do not explore dynamically linked
	 * libraries right now.
	 */
	if ((bbi.bb->branchType == Instruction::BT_CALL) or
	    (bbi.bb->branchType == Instruction::BT_JUMP_UNCOND)) {
		DEBUG(std::cout << "Checking for dynamic branches: " << std::hex << bbi.targets[0].v << std::endl;);

		if (isDynamicBranch(bbi.targets[0])) {
			DEBUG(std::cout << "skipping dynamic library jump to " << std::hex << bbi.targets[0].v << std::endl;);

			bbi.targets.clear();

			bbi.bb->branchType = Instruction::BT_JUMP_UNCOND;
			Address newTarget = bbi.bb->lastInstruction();
			newTarget += bbi.bb->instructions.back()->length();
			bbi.targets.push_back(newTarget);
		}
	}

	DEBUG(std::cout << "Finished. BB instructions: " << bbi.bb->firstInstruction().v
	                << " - " << bbi.bb->lastInstruction().v << std::endl;);
	return bbi;
}


CFGVertexDescriptor CFGBuilder_priv::handleIncomingEdges(CFGVertexDescriptor prevVertex,
                                                         CFGVertexDescriptor newVertex,
                                                         BBInfo& bbi)
{
	CFGVertexDescriptor retVD = newVertex;
	DEBUG(std::cout << "Adding incoming edges to " << bbi.bb->firstInstruction().v
	                << "..." << std::endl;);

	/* update function entry information */
	if ((prevVertex == 0) or (cfg(prevVertex).bb->branchType == Instruction::BT_CALL)) {
		cfg(newVertex).functionEntry = newVertex;
	} else {
		cfg(newVertex).functionEntry = cfg(prevVertex).functionEntry;
	}

	/* We need to add an edge from the previous vd */
	addCFGEdge(prevVertex, newVertex);
	updateReturnsForCall(prevVertex, newVertex);

	/* Update the call dominator map */
	updateCallDoms(prevVertex, newVertex);
	updateRetDoms(newVertex);

	/*
	 * On incoming edges we might detect that we need to split the current block. This is done in
	 * three steps:
	 *
	 * 1) We perform standard handling for adding incoming edges on the initial BB (BB0) and keep
	 *    track of the points where we need to split the BB. (There may be multiple such points.)
	 * 2) We split BB0 into multiple parts with the respective connections: BB0 -> BB1 -> BB2
	 * 3) We continue working with the last BB (BB2 in the example) as this is the one that needs
	 *    handling w.r.t. outgoing edges.
	 */
	std::vector<UnresolvedLink> splitPoints;

	auto AddressIsInBB = [&] (UnresolvedLink& u)
	{ return (bbi.bb->firstInstruction() <= u.second) and (u.second <= bbi.bb->lastInstruction()); };

	PendingResolutionList::iterator n =
		std::find_if(_bb_connections.begin(), _bb_connections.end(), AddressIsInBB);
	while (n != _bb_connections.end()) {
		BasicBlock *b = cfg((*n).first).bb;
		DEBUG(std::cout << "Unresolved link: " << b->firstInstruction().v << " -> "
		                << (*n).second.v << std::endl;);
		/* If the unresolved link goes to our start address, add an edge, ... */
		if ((*n).second == bbi.bb->firstInstruction()) {
			addCFGEdge((*n).first, newVertex);
			updateReturnsForCall((*n).first, newVertex);
		} else {
			/*
			 * Otherwise, we found a jump that goes into the middle of this
			 * newly found BB, requiring a split. However, we first collect
			 * split points (because other pending links may still point to
			 * the start address) and handle them below.
			 */
			splitPoints.push_back(*n);
		}
		_bb_connections.erase(n);
		n = std::find_if(_bb_connections.begin(), _bb_connections.end(), AddressIsInBB);
	}

	if (splitPoints.size() >= 2) {
		/* Sort the split points w.r.t. the split address */
		std::sort(splitPoints.begin(), splitPoints.end(),
				 [] (CFGBuilder_priv::UnresolvedLink const & l1, CFGBuilder_priv::UnresolvedLink const & l2)
				 { return l1.second < l2.second; }
		);
		if (Configuration::get()->debug) {
			std::cout << "Splitting BB from incoming links:" << std::endl;
			BOOST_FOREACH(UnresolvedLink l, splitPoints) {
				std::cout << l.first << " -> " << l.second.v << std::endl;
			}
		}
	}

	while (!splitPoints.empty()) {
		UnresolvedLink& link                = splitPoints.front();
		//CFGVertexDescriptor source          = link.first;
		Address splitAddress                = link.second;
		CFGVertexDescriptor splitTailVertex = splitBasicBlock(newVertex, splitAddress);
		assert(splitTailVertex != newVertex);

		/*
		 * splitPoints vector is sorted (see above). Therefore, we can now remove all
		 * subsequent elements with the same target address, because they all go to
		 * the same new BB.
		 */
		do {
			addCFGEdge(link.first, splitTailVertex);
			link = splitPoints.front();
			splitPoints.erase(splitPoints.begin());
		} while ((splitPoints.size() > 0) and (link.second == splitAddress));

		newVertex = splitTailVertex;

		/*
		 * Next, we want to continue working on the tail of the BB chain. Everything upfront
		 * is done.
		 */
		retVD = splitTailVertex;
	}
	return retVD;
}


void CFGBuilder_priv::handleOutgoingEdges(BBInfo& bbi, CFGVertexDescriptor newVertex)
{
	DEBUG(std::cout << "Adding outgoing targets." << std::endl;);
	if (Configuration::get()->debug) {
		std::cout << "   [";
		BOOST_FOREACH(Address a, bbi.targets) {
			std::cout << a.v << ",";
		}
		std::cout << "]" << std::endl;
	}

	/* Now establish target links */
	while (!bbi.targets.empty()) {
		Address a = bbi.targets.front();
		bbi.targets.erase(bbi.targets.begin());
		DEBUG(std::cout << "Next outgoing target address: " << a.v << std::endl;);

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
			targetNode = findCFGNodeWithAddress(_cfg, a, newVertex);
		} catch (NodeNotFoundException) {
			DEBUG(std::cout << "This is no BB I know about yet. Queuing 0x" << a.v
			                << " for discovery." << std::endl;);
			// case 3: need to discover more code first
			_bb_connections.push_back(UnresolvedLink(newVertex, a));
			continue;
		}

		if (a == cfg(targetNode).bb->firstInstruction()) {
			DEBUG(std::cout << "jump goes to beginning of BB. Adding CFG edge." << std::endl;);
			addCFGEdge(newVertex, targetNode);
			updateReturnsForCall(newVertex, targetNode);
			addReturnEdgesForCall(newVertex, targetNode);
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
				bbi2.bb      = cfg(splitTailVertex).bb;
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

	/*
	 * Special handling for outgoing edges of a RET basic block. Here, the disassembler
	 * cannot tell us any return address, so we need to settle for the data we kept on the fly.
	 */
	if (bbi.bb->branchType == Instruction::BT_RET) {
		/* 1. Find the dominator, that is the node that has been called to reach this bb. */
		CFGVertexDescriptor callee = _callDoms[bbi.bb->firstInstruction()];
		DEBUG(std::cout << "callee == " << callee << std::endl;);
		if (callee == 0) return;

		CFGNodeInfo& cNode = cfg(callee);
		std::set<Address>& returns = _callRets[cNode.bb->firstInstruction()];
		BOOST_FOREACH(Address a, returns) {
			DEBUG(std::cout << "return to " << a.v << std::endl;);
			CFGVertexDescriptor targetNode;
			try {
				targetNode = findCFGNodeWithAddress(_cfg, a, newVertex);
			} catch (NodeNotFoundException) {
				DEBUG(std::cout << "This is no BB I know about yet. Queuing 0x" << a.v
					            << " for discovery." << std::endl;);
				// case 3: need to discover more code first
				_bb_connections.push_back(UnresolvedLink(newVertex, a));
				continue;
			}

			if (a == cfg(targetNode).bb->firstInstruction()) {
				DEBUG(std::cout << "jump goes to beginning of BB. Adding CFG edge." << std::endl;);
				addCFGEdge(newVertex, targetNode);
			} else {
				throw ThisShouldNeverHappenException("Return into middle of a BB?");
			}
		}
	}
}


CFGVertexDescriptor CFGBuilder_priv::splitBasicBlock(CFGVertexDescriptor splitVertex, Address splitAddress)
{
	// 1) create new empty BB
	BasicBlock* bb2       = new BasicBlock();
	CFGNodeInfo& bbNode   = cfg(splitVertex);

	/*
	 * Branch type: new BB inherits the old one's.
	 * Old BB becomes a JUMP_UNCOND node.
	 */
	bb2->branchType       = bbNode.bb->branchType;
	bbNode.bb->branchType = Instruction::BT_JUMP_UNCOND;

	DEBUG(std::cout << "split: new BB @ " << bb2 << std::endl;);

	/* Find the first instruction of the new BB. */
	std::vector<Instruction*>::iterator iit = std::find_if(bbNode.bb->instructions.begin(),
	                                                       bbNode.bb->instructions.end(),
	                                          [&] (Instruction* i) { return i->ip() == splitAddress; });
	/*
	 * We came here because someone deemed us to split the BB. Hence, we must
	 * assume that the split instruction is within the BB!
	 */
	assert(iit != bbNode.bb->instructions.end());

	/*
	 * Now move the new BB's instructions over to the new container.
	 */
	while (iit != bbNode.bb->instructions.end()) {
		DEBUG(std::cout << "split: moving instr. @ " << (*iit)->ip().v
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
	updateCallDoms(splitVertex, vert2);

	return vert2;
}
