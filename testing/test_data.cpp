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

#include "data/memory.h"
#include "data/input.h"

#include "instruction/disassembler.h"

#include "wvtest.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <iostream>

WVTEST_MAIN("memregion")
{
	MemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	
	WVPASSEQ(r.base, 0xf000);
	WVPASSEQ(r.size, 0x0fff);
	WVPASS(r.contains(r.base));
	WVPASS(r.contains(r.base + r.size));
	WVPASS(r.contains(r.base + 5));
	WVFAIL(r.contains(r.base-1));
	WVFAIL(r.contains(r.base + r.size + 1));
}


WVTEST_MAIN("reloc memregion")
{
	RelocatedMemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	r.mappedBase = 0x10000;
	
	WVPASSEQ(r.base, 0xf000);
	WVPASSEQ(r.size, 0x0fff);
	WVPASSEQ(r.mappedBase, 0x10000);

	WVPASS(r.contains(r.base));
	WVPASS(r.contains(r.base + r.size));
	WVPASS(r.contains(r.base + 5));
	WVFAIL(r.contains(r.base-1));
	WVFAIL(r.contains(r.base + r.size + 1));
	
	WVPASS(r.relocContains(0x10200));
	WVPASSEQ(r.regionToReloc(0xF200),  0x10200);
	WVPASSEQ(r.relocToRegion(0x10200), 0xF200);
}


WVTEST_MAIN("hex input reader")
{
	HexbyteInputReader ir;
	HexbyteInputReader ir2;
	char const *in[]  =  {"12", "34", "56", "78", "90", "de", "ad", "be", "ef"};
	char const *in2[] =  {"0f", "f0", "ba", "xy", "12"};

	for (unsigned i = 0; i < 9; ++i) {
		ir.addData(in[i]);
	}
	WVPASSEQ(ir.sectionCount(), 1);
	WVPASSEQ(static_cast<int>(ir.section(0)->bytes()), 9);

	for (unsigned i = 0; i < 5; ++i) {
		ir2.addData(in2[i]);
	}
	WVPASSEQ(ir.sectionCount(), 1);
	WVPASSEQ(static_cast<int>(ir2.section(0)->bytes()), 4);

	uint8_t const * const ptr = reinterpret_cast<uint8_t const * const>(ir.section(0)->getBuffer().base) + 5;
	WVPASSEQ(*(ptr  ), 0xde);
	WVPASSEQ(*(ptr+1), 0xad);
	WVPASSEQ(*(ptr+2), 0xbe);
	WVPASSEQ(*(ptr+3), 0xef);
}


WVTEST_MAIN("hex input, large")
{
	HexbyteInputReader hr;
	char const *in[] = {"c3"};
	for (unsigned i = 0; i < 3000; ++i) {
		hr.addData(in[0]);
	}
	WVPASSEQ(hr.sectionCount(), 1);
	WVPASSEQ(hr.entry(), 0);
	WVPASSEQ(static_cast<int>(hr.section(0)->bytes()), 3000);
}

WVTEST_MAIN("file input reader")
{
	char const *file = "testing/testcases/payload.bin";
	FileInputReader fr;
	fr.addData(file);
	WVPASSEQ(fr.sectionCount(), 1);
	WVPASSEQ(static_cast<int>(fr.section(0)->bytes()), 32);
}
