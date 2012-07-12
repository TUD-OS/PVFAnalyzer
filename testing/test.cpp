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

#include "memory.h"
#include "input.h"
#include "wvtest.h"

#include <iostream>

static void
memregion()
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


static void
relocmemregion()
{
	RelocatedMemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	r.mapped_base = 0x10000;
	
	WVPASSEQ(r.base, 0xf000);
	WVPASSEQ(r.size, 0x0fff);
	WVPASSEQ(r.mapped_base, 0x10000);

	WVPASS(r.contains(r.base));
	WVPASS(r.contains(r.base + r.size));
	WVPASS(r.contains(r.base + 5));
	WVFAIL(r.contains(r.base-1));
	WVFAIL(r.contains(r.base + r.size + 1));
	
	WVPASS(r.reloc_contains(0x10200));
	WVPASSEQ(r.region_to_reloc(0xF200),  0x10200);
	WVPASSEQ(r.reloc_to_region(0x10200), 0xF200);
}


static void
hexinput()
{
	RawData is, is2;
	HexbyteInputReader ir(&is);
	HexbyteInputReader ir2(&is2);
	char const *in[]  =  {"12", "34", "56", "78", "90", "de", "ad", "be", "ef"};
	char const *in2[] =  {"0f", "f0", "ba", "xy", "12"};

	for (unsigned i = 0; i < 9; ++i) {
		ir.addData(in[i]);
	}
	WVPASSEQ(static_cast<int>(is.bytes()), 9);

	for (unsigned i = 0; i < 5; ++i) {
		ir2.addData(in2[i]);
	}
	WVPASSEQ(static_cast<int>(is2.bytes()), 4);

	uint8_t const * const ptr = is.getPtr(5);
	WVPASSEQ(*(ptr  ), 0xde);
	WVPASSEQ(*(ptr+1), 0xad);
	WVPASSEQ(*(ptr+2), 0xbe);
	WVPASSEQ(*(ptr+3), 0xef);
}


static void
hexinput_large()
{
	RawData rd;
	HexbyteInputReader hr(&rd);
	char const *in[] = {"c3"};
	for (unsigned i = 0; i < 3000; ++i) {
		hr.addData(in[0]);
	}
	WVPASSEQ(static_cast<int>(rd.bytes()), 3000);
}

static void
fileinput()
{
	RawData is;
	char const *file = "testcases/payload.bin";
	FileInputReader fr(&is);
	fr.addData(file);
	WVPASSEQ(static_cast<int>(is.bytes()), 32);
}


int main()
{
	memregion();
	relocmemregion();
	hexinput();
	hexinput_large();
	fileinput();

	std::cout << "\033[32m---> all tests finished. <---\033[0m" << std::endl;
	return 0;
}
