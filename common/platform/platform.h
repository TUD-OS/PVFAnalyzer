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

#include "util.h"

class Platform
{
public:
	virtual ~Platform()  { }
	virtual int numGPRs() = 0;
};

#include <udis86.h>
#include <boost/graph/graph_concepts.hpp>

class PlatformX8632 : public Platform
{
public:
	enum Register {
		EAX, EBX, ECX, EDX, EFLAGS,
		ESP, EIP, ESI, EDI, EBP,
		REGMAX
	};

	virtual ~PlatformX8632() { }

	virtual int numGPRs()
	{
		return REGMAX;
	}

	static Register UdisToPlatformRegister(unsigned udreg)
	{
		switch (udreg) {
			case UD_R_EAX: case UD_R_AX: case UD_R_AL: case UD_R_AH:
				return EAX;
			case UD_R_EBX: case UD_R_BX: case UD_R_BL: case UD_R_BH:
				return EBX;
			case UD_R_ECX: case UD_R_CX: case UD_R_CL: case UD_R_CH:
				return ECX;
			case UD_R_EDX: case UD_R_DX: case UD_R_DL: case UD_R_DH:
				return EDX;
			case UD_R_ESP: return ESP;
			case UD_R_EBP: return EBP;
			case UD_R_ESI: return ESI;
			case UD_R_EDI: return EDI;
			default:
				std::cout << __func__ << " UNHANDLED REGNO: " << std::hex << udreg << std::endl;
				return REGMAX;
		}
	}

	static unsigned UdisToPlatformAccessSize(unsigned udreg)
	{
		switch(udreg) {
			case UD_R_EAX: case UD_R_EBX: case UD_R_ECX: case UD_R_EDX:
			case UD_R_ESP: case UD_R_EBP: case UD_R_ESI: case UD_R_EDI:
				return 32;
			case UD_R_AX: case UD_R_BX: case UD_R_CX: case UD_R_DX:
				return 16;
			case UD_R_AL: case UD_R_BL: case UD_R_CL: case UD_R_DL:
			case UD_R_AH: case UD_R_BH: case UD_R_CH: case UD_R_DH:
				return 8;
		}

		throw NotImplementedException(__func__);
	}
};