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
#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/foreach.hpp>  // FOREACH
#include <fstream>

#include "instruction/basicblock.h"
#include "data/input.h"

/**
 * @brief CFG Node data
 **/
struct CFGNodeInfo
{
	BasicBlock* bb;         ///> Basic Block this CFG node represents
	unsigned functionEntry; ///> BB that contains return info for this BB
	std::vector<unsigned> retNodes; ///> return nodes (only valid if node is a function entry node)

	CFGNodeInfo(BasicBlock *b = 0)
		: bb(b), functionEntry(0), retNodes()
	{ }

	CFGNodeInfo(CFGNodeInfo const& n) = default;
	CFGNodeInfo& operator=(CFGNodeInfo const& n) = default;

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		a & bb;
		a & functionEntry;
		a & retNodes;
	}

	Address returnTargetAddress() const
	{
		if (!bb or bb->instructions.empty())
			return Address(0);
		Instruction* inst = bb->instructions.back();
		return inst->ip() + inst->length();
	}
};


/* CFG typedef */
typedef boost::adjacency_list<boost::vecS,
                              boost::vecS,
                              boost::bidirectionalS,
                              CFGNodeInfo> ControlFlowGraph;

typedef boost::graph_traits<ControlFlowGraph>::vertex_descriptor CFGVertexDescriptor;
typedef boost::graph_traits<ControlFlowGraph>::vertex_iterator   CFGVertexIterator;
typedef boost::graph_traits<ControlFlowGraph>::edge_descriptor   CFGEdgeDescriptor;
typedef boost::graph_traits<ControlFlowGraph>::edge_iterator     CFGEdgeIterator;


/**
 * @brief CFG Creator
 **/
class CFGBuilder
{
protected:
	std::map<Address, bool> _terminators;
public:
	/**
	 * @brief Create a new builder for the given CFG and input sections.
	 *
	 * @param input filled in input sections
	 * @param cfg CFG to build/extend
	 * @return CFGBuilder*
	 **/
	static CFGBuilder* get(std::vector<InputReader*> const & input, ControlFlowGraph& cfg);

	virtual ~CFGBuilder() { }

	/**
	 * @brief Analyse and build CFG from input.
	 *
	 * @param entry Entry point address to start analysing at.
	 * @return void
	 **/
	virtual void build(Address entry) = 0;

	void terminators(std::vector<Address> addresses)
	{
		BOOST_FOREACH(Address a, addresses) {
			_terminators[a] = true;
		}
	}
};


/**
 * @brief Free the dynamically allocated memory associated with a CFG
 *
 * CFGs store node information that has been dynamically allocated. This
 * function iterates over a CFG's nodes and deletes this dynamically allocated
 * memory.
 *
 * @param cfg Control Flow Graph
 * @return void
 **/
void freeCFGNodes(ControlFlowGraph const &cfg);


/**
 * @brief Serialize CFG into a file.
 *
 * @param cfg CFG
 * @param name file name
 * @return void
 **/
void CFGToFile(ControlFlowGraph const & cfg, std::string const &name);


/**
 * @brief Read CFG from a file
 *
 * @param cfg CFG
 * @param name file name
 * @return void
 **/

void CFGFromFile(ControlFlowGraph& cfg, std::string const &name);


/**
 * @brief Node writer for Graphviz CFG output
 **/
struct GraphvizInstructionWriter
{
	ControlFlowGraph& g;

	GraphvizInstructionWriter(ControlFlowGraph& _g)
		: g(_g)
	{ }

	void operator() (std::ostream& out, const CFGVertexDescriptor &v) const;
};


CFGVertexDescriptor const
findCFGNodeWithAddress(ControlFlowGraph const & cfg, Address a, CFGVertexDescriptor startSearch = 0);
