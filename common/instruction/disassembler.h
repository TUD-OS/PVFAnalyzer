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

#include <cstdint>
#include <udis86.h>

#include "data/memory.h"
#include "instruction/instruction.h"

/**
 * @brief Disassembler interface
 * 
 * A disassembler generically represents a buffer containing
 * instruction bytes and provides functionality to obtain
 * decoded Instructions from within this buffer.
 * 
 **/
class Disassembler
{
public:
	Disassembler()
		: _buffer()
	{ }

	virtual ~Disassembler()
	{ }

	/**
	 * @brief Disassemble next instruction at given offset
	 *
	 * @param offset offset within underlying buffer to start decoding at
	 * @return Instruction*
	 **/
	virtual Instruction* disassemble(Address offset) = 0;

	/*
	 * Buffer management
	 *
	 * The disassembler works on an in-memory buffer containing
	 * the instruction bytes. As instruction decoding may depend on
	 * the memory location (e.g., relative addressing), this buffer
	 * needs to have relocation information attached.
	 */
	RelocatedMemRegion buffer()           { return _buffer; }
	virtual void buffer(RelocatedMemRegion region) { _buffer = region; }

protected:
	RelocatedMemRegion _buffer; // underlying buffer

private:
	Disassembler(const Disassembler&) : _buffer() { }
	Disassembler& operator=(const Disassembler&) { return *this; }
};


class Udis86Disassembler : public Disassembler
{
public:
	virtual ~Udis86Disassembler() { }
	virtual Instruction* disassemble(Address a);
};