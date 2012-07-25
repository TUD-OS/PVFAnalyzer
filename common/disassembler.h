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

#include "memory.h"

class Disassembler
{
public:
	Disassembler()
		: _buffer()
	{ }

	virtual ~Disassembler()
	{ }

	virtual unsigned disassemble() = 0;

	MemRegion buffer() { return _buffer; }
	virtual void buffer(MemRegion region) { _buffer = region; }

protected:
	MemRegion _buffer;

private:
	Disassembler(const Disassembler&) : _buffer() { }
	Disassembler& operator=(const Disassembler&) { return *this; }
};


class Instruction
{
protected:
	char *bytes;
};


class Udis86Disassembler : public Disassembler
{
public:
	Udis86Disassembler();

	virtual ~Udis86Disassembler()
	{ }

	virtual unsigned disassemble();

	virtual void buffer(MemRegion region)
	{
		Disassembler::buffer(region);
		ud_set_input_buffer(&_ud_obj, reinterpret_cast<uint8_t*>(region.base), region.size);
	}
private:
	ud_t _ud_obj;

	Udis86Disassembler(const Udis86Disassembler& )
		: _ud_obj()
	{ }

	Udis86Disassembler& operator=(const Udis86Disassembler&) { return *this; }
};