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

#include "instruction.h"

/**
 * @brief CFG Node data
 **/
struct CFGNodeInfo
{
	Instruction* instruction; // right now we only store an instruction

	CFGNodeInfo(Instruction *inst = 0)
		: instruction(inst)
	{ }

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		a & instruction;
	}
};

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS,
                              boost::bidirectionalS,
                              CFGNodeInfo> ControlFlowGraph;

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
void freeCFGNodes(ControlFlowGraph &cfg)
{
	boost::graph_traits<ControlFlowGraph>::vertex_iterator vi, vi_end;
	for (boost::tie(vi, vi_end) = boost::vertices(cfg);
		 vi != vi_end; ++vi) {
		if (cfg[*vi].instruction) {
			delete cfg[*vi].instruction;
		}
	}
}


/**
 * @brief Node writer for Graphviz CFG output
 **/
struct GraphvizInstructionWriter
{
	ControlFlowGraph& g;

	GraphvizInstructionWriter(ControlFlowGraph& _g)
		: g(_g)
	{ }

	template <class Vertex>
	void operator() (std::ostream& out, const Vertex &v) const
	{
		out << " [shape=box,";
		if (g[v].instruction) {
			out << "label=\"\\[0x" << std::hex << std::setw(8)
			    << g[v].instruction->ip() << "\\]\\n"
			    << g[v].instruction->c_str() << "\"";
		} else {
			out << "label=\"<empty>\"";
		}
		out << "]";
	}
};
