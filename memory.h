#pragma once

#include <cstdint>
#include <cstddef>
#include <cassert>

typedef ptrdiff_t address;

/**
 * @brief Arbitrary memory region
 **/
struct MemRegion
{
	address   base;
	ptrdiff_t size;
};


/**
 * @brief Memory region with relocation info
 * 
 * This kind of region can be mapped to an address different from the one
 * it is supposed to be mapped to. Think e.g., of an ELF binary section that
 * should be mapped to address 0xF000, but the current analyzer cannot map to
 * this location. Then, we can map it to some other address 0xA000 and use
 * a RelocatedMemRegion to translate between current virtual addresses and
 * originally intended virtual addresses.
 **/
struct RelocatedMemRegion : public MemRegion
{
	address   mapped_base;
		
	/**
	 * @brief Translate a relocated address to a region address.
	 *
	 * @param a Address within [mapped_base, mapped_base+size]
	 * @return address within [base, base+size]
	 **/
	address reloc_to_region(address const & a) const
	{
		assert(mapped_base <= a);
		assert(a <= mapped_base + size);
		return a - mapped_base + base;
	}
	
	/**
	 * @brief Translate an unrelocated address to a relocated one.
	 *
	 * @param a Address within [base, base+size]
	 * @return address within [mapped_base, mapped_base+size]
	 **/
	address region_to_reloc(address const & a) const
	{
		assert(base <= a);
		assert(a <= base + size);
		return a - base + mapped_base;
	}
};