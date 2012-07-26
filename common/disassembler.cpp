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
	Address unreloc      = _buffer.base + offset;
	i->membase(unreloc);
	i->ip(_buffer.region_to_reloc(unreloc));

	ud_disassemble(i->ud_obj());
	return i;
}