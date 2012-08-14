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
#include "instruction/disassembler.h"
#include "instruction/instruction_udis86.h"
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
	i->ip(_buffer.regionToReloc(unreloc));

	ud_disassemble(i->udObj());
	return i;
}