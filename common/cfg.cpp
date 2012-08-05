#include "instruction/cfg.h"

void freeCFGNodes(ControlFlowGraph &cfg)
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
	boost::archive::binary_oarchive oa(ofs);
	oa << cfg;
}

void CFGFromFile(ControlFlowGraph& cfg, std::string const &name)
{
	std::ifstream ifs(name);
	boost::archive::binary_iarchive ia(ifs);
	ia >> cfg;
}