#include "disassembler.h"
#include "instruction.h"
#include <iostream>

Instruction* Udis86Disassembler::disassemble(Address offset)
{
	if (_buffer == MemRegion()) // no memregion set?
		return 0;

	if (offset >= _buffer.size)
		return 0;

	Udis86Instruction* i = new Udis86Instruction();
	i->membase(_buffer.base + offset);
	// XXX: need to set EIP properly!
	i->ip(_buffer.base + offset);

	ud_disassemble(i->ud_obj());
	//std::cout << ud_insn_asm(&_ud_obj) << std::endl;
	return i;
}