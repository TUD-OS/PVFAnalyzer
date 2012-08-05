#include "disassembler.h"
#include "instruction_udis86.h"
#include <iostream>

Instruction* Udis86Disassembler::disassemble(Address offset)
{
	if (_buffer == MemRegion()) // no memregion set?
		return 0;

	if (offset >= _buffer.size)
		return 0;

	Udis86Instruction* i = new Udis86Instruction();

	/*
	 * Only the disassembler holds the _full_ buffer containing all bytes
	 * to be decoded. As Instructions only represent a single opcode, we
	 * only mark a small subset of the real buffer as their input buffer.
	 * This allows serializing instruction bytes along with the instructions
	 * later on.
	 */
	Address unreloc      = _buffer.base + offset;
	i->membase(unreloc);
	i->ip(_buffer.region_to_reloc(unreloc));

	ud_disassemble(i->ud_obj());
	return i;
}