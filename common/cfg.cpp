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

void freeCFGNodes(ControlFlowGraph const &cfg)
{
	CFGVertexIterator vi, vi_end;
	for (boost::tie(vi, vi_end) = boost::vertices(cfg);
		 vi != vi_end; ++vi) {
		if (cfg[*vi].bb) {
			delete cfg[*vi].bb;
		}
	}
}

void CFGToFile(ControlFlowGraph const & cfg, std::string const &name)
{
	std::ofstream ofs(name);
	if (!ofs.good())
		throw FileNotFoundException(name.c_str());
	boost::archive::binary_oarchive oa(ofs);
	oa << cfg;
}

void CFGFromFile(ControlFlowGraph& cfg, std::string const &name)
{
	std::ifstream ifs(name);
	if (!ifs.good())
		throw FileNotFoundException(name.c_str());
	boost::archive::binary_iarchive ia(ifs);
	ia >> cfg;
}

void GraphvizInstructionWriter::operator() (std::ostream& out, const CFGVertexDescriptor &v) const
{
	out << " [shape=box,fontname=Terminus,";
	BasicBlock* bb = g[v].bb;
	if (!bb->instructions.empty()) {
		out << "label=\"[@0x";
		out << std::hex << bb->firstInstruction() << "]\\l";
		BOOST_FOREACH(Instruction* i, bb->instructions) {
			out << i->c_str() << "\\l";
		}
		out << "\"";
	} else {
		out << "label=\"<empty>\"";
	}
	out << "]";
}
