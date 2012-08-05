#include "instruction/basicblock.h"
#include "instruction/cfg.h"
#include "instruction/disassembler.h"

#include <boost/foreach.hpp>

class CFGBuilder_priv : public CFGBuilder
{
	Udis86Disassembler dis;
	ControlFlowGraph   cfg;

	void addSingleBuffer(RelocatedMemRegion const& buffer);

public:
	CFGBuilder_priv()
		: dis(), cfg()
	{ }

	virtual void build(std::vector<InputReader*> v);
};

CFGBuilder* CFGBuilder::get()
{
	static CFGBuilder_priv p;
	return &p;
}


/*
 * Basic block construction
 *
 * 1) Start with an empty BB
 * 2) Iterate over input until
 *    a) the input buffer is empty -> BB ends here, no new BBs discovered
 *    b) a branch instruction is hit
 *       -> BB ends here
 *       -> new BBs are to be discovered:
 *          a) JMP/CALL instruction:
 *             - jump target _and_ instruction following the last one become
 *               new BB beginnings
 *             - connections to those two new BBs are added to CFG
 *          b) RET: no new BB
 *             - connection to return address is added
 *          c) SYSENTER etc.: no new BB, no connection (XXX: but maybe special mark?)
 * 3) JMP targets may go into the middle of an already discovered BB
 *    -> split the BB into two, add respective connections
 */
void CFGBuilder_priv::build ( std::vector< InputReader* > v )
{
	BOOST_FOREACH(InputReader* ir, v) {
		for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
			addSingleBuffer(ir->section(sec)->getBuffer());
		}
	}
}


void CFGBuilder_priv::addSingleBuffer ( const RelocatedMemRegion& buffer )
{
	std::cerr << __func__ << ": IMPLEMENT ME" << std::endl;
}