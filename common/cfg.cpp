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

void ControlFlowGraph::releaseNodeMemory()
{
	CFGVertexIterator vi, vi_end;
	for (boost::tie(vi, vi_end) = boost::vertices(cfg);
		 vi != vi_end; ++vi) {
		if (node(*vi).bb) {
			delete node(*vi).bb;
		}
	}
}


void ControlFlowGraph::toFile(std::string const& name)
{
	std::ofstream ofs(name);
	if (!ofs.good())
		throw FileNotFoundException(name.c_str());
	boost::archive::binary_oarchive oa(ofs);
	oa << cfg;
	oa << terminators;
}


void ControlFlowGraph::fromFile(std::string const &name)
{
	std::ifstream ifs(name);
	if (!ifs.good())
		throw FileNotFoundException(name.c_str());
	boost::archive::binary_iarchive ia(ifs);
	ia >> cfg;
	ia >> terminators;
}


void GraphvizInstructionWriter::operator() (std::ostream& out, const CFGVertexDescriptor &v) const
{
	out << " [shape=box,fontname=Terminus,";
	BasicBlock* bb = g.node(v).bb;
	if (!bb->instructions.empty()) {
		out << "label=\"[@0x";
		out << std::hex << bb->firstInstruction().v << "]\\l";
		BOOST_FOREACH(Instruction* i, bb->instructions) {
			out << i->c_str() << "\\l";
		}
		out << "\"";
	} else {
		out << "label=\"<empty>\"";
	}
	out << "]";
}
