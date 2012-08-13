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
	BasicBlock* bb; // right now we only store an instruction

	CFGNodeInfo(BasicBlock *b = 0)
		: bb(b)
	{ }

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		a & bb;
	}
};

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS,
                              boost::bidirectionalS,
                              CFGNodeInfo> ControlFlowGraph;

typedef boost::graph_traits<ControlFlowGraph>::vertex_descriptor CFGVertexDescriptor;
typedef boost::graph_traits<ControlFlowGraph>::vertex_iterator   CFGVertexIterator;


class CFGBuilder
{
public:
	static CFGBuilder* get(std::vector<InputReader*> const & input, ControlFlowGraph& cfg);

	virtual ~CFGBuilder() { }

	virtual void build(Address entry) = 0;
	/*virtual ControlFlowGraph const& cfg()   = 0;*/
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
void CFGToFile(ControlFlowGraph const & cfg, std::string const &name);
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
