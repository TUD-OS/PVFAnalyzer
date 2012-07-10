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

class Disassembler
{
public:
	Disassembler()
	{
	}

	virtual ~Disassembler()
	{
	}

	virtual void disassemble(uint8_t const *buf) = 0;

protected:

private:
	Disassembler(const Disassembler&) { }
	Disassembler& operator=(const Disassembler&) { return *this; }
};

class Udis86Disassembler : public Disassembler
{
public:
	Udis86Disassembler();

	virtual ~Udis86Disassembler()
	{
	}

	virtual void disassemble(uint8_t const *buf);
private:
	Udis86Disassembler(const Udis86Disassembler& ) { }
	Udis86Disassembler& operator=(const Udis86Disassembler&) { return *this; }
};