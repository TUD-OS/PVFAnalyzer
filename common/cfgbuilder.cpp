// vi:ft=cpp
#include "instruction/basicblock.h"
#include "instruction/cfg.h"
#include "instruction/disassembler.h"
#include "version.h"

#include <boost/foreach.hpp>

class CFGBuilder_priv : public CFGBuilder
{
	Udis86Disassembler dis;
	ControlFlowGraph   cfg;

	std::vector<InputReader*> const &inputs;

	BasicBlock* generate(Address e);

	RelocatedMemRegion bufferForAddress(Address a)
	{
		VERBOSE(std::cerr << __func__ << "(" << a << ")" << std::endl;);
		BOOST_FOREACH(InputReader *ir, inputs) {
			for (unsigned sec = 0; sec < ir->section_count(); ++sec) {
				RelocatedMemRegion mr = ir->section(sec)->getBuffer();
				if (mr.reloc_contains(a))
					return mr;
			}
		}
		return RelocatedMemRegion();
	}

public:
	CFGBuilder_priv(std::vector<InputReader*> const& in)
		: dis(), cfg(), inputs(in)
	{ }

	virtual void build(Address a);
};

CFGBuilder* CFGBuilder::get(std::vector<InputReader*> const& in)
{
	static CFGBuilder_priv p(in);
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
void CFGBuilder_priv::build (Address entry)
{
	VERBOSE(std::cout << __func__ << "(" << std::hex << entry << ")" << std::endl;);
	BasicBlock* bb = generate(entry);
	(void)bb;
}


BasicBlock* CFGBuilder_priv::generate(Address e)
{
	BasicBlock *r          = new BasicBlock();
	(void)r;
	RelocatedMemRegion buf = bufferForAddress(e);

	if (buf.size == 0)
		return 0;

	unsigned offs  = e - buf.mapped_base;
	Instruction *i = 0;

	dis.buffer(buf);
	while ((i = dis.disassemble(offs)) != 0) {
		VERBOSE(i->print(); std::cout << "\n";);
		offs += i->length();
	}

	return 0;
}
