#include "memory.h"

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


int main()
{
	test_memregion();
	test_relocmemregion();
	
	std::cout << "all tests finished." << std::endl;
	
	return 0;
}
