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

#include <iostream>
#include <cassert> // for now

static void
test_memregion()
{
	MemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	
	assert(r.base == 0xf000);
	assert(r.size == 0x0fff);
	assert(r.contains(r.base));
	assert(r.contains(r.base + r.size));
	assert(r.contains(r.base + 5));
	assert(!r.contains(r.base-1));
	assert(!r.contains(r.base + r.size + 1));
}


static void
test_relocmemregion()
{
	RelocatedMemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	r.mapped_base = 0x10000;
	
	assert(r.base == 0xF000);
	assert(r.size == 0x0FFF);
	assert(r.mapped_base == 0x10000);
	
	assert(r.contains(r.base));
	assert(r.contains(r.base + r.size));
	assert(r.contains(r.base + 5));
	assert(!r.contains(r.base-1));
	assert(!r.contains(r.base + r.size + 1));
	
	assert(r.reloc_contains(0x10200));
	assert(r.region_to_reloc(0xF200) == 0x10200);
	assert(r.reloc_to_region(0x10200) == 0xF200);
}


static void
test_hexinput()
{
	InputStream is, is2;
	InputReader *ir   = new HexbyteInputReader(&is);
	InputReader *ir2  = new HexbyteInputReader(&is2);
	char const *in[]  =  {"12", "34", "56", "78", "90", "de", "ad", "be", "ef"};
	char const *in2[] =  {"0f", "f0", "ba", "xy", "12"};

	for (unsigned i = 0; i < 9; ++i) {
		ir->addData(in[i]);
	}
	assert(is.bytes() == 9);

	for (unsigned i = 0; i < 5; ++i) {
		ir2->addData(in2[i]);
	}
	assert(is2.bytes() == 4);
}


int main()
{
	test_memregion();
	test_relocmemregion();
	test_hexinput();
	std::cout << "all tests finished." << std::endl;
	return 0;
}
