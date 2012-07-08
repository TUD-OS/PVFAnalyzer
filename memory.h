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
#include <cstddef>
#include <cassert>

typedef ptrdiff_t address; // address type

/**
 * @brief Arbitrary memory region
 **/
struct MemRegion
{
	MemRegion()
		: base(0), size(0)
	{ }
	
	virtual ~MemRegion() { }
	
	/**
	 * @brief Region base address
	 **/
	address   base;
	
	/**
	 * @brief Region size
	 **/
	ptrdiff_t size;
	
	/**
	 * @brief Check if address a is within the range specified by the region.
	 *
	 * @param a Address
	 * @return true if a is within bounds, false otherwise
	 **/
	bool contains(address const a) const
	{
		return (this->base <= a) && (a <= (this->base + this->size));
	}
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
	/**
	 * @brief Address this region is mapped to
	 **/
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
	
	/**
	 * @brief Determine if the given address is within the relocated region.
	 *
	 * @param a address
	 * @return true if a is witihn [mapped_base, mapped_base+size], false otherwise
	 **/
	bool reloc_contains(address const a) const
	{
		return contains(reloc_to_region(a));
	}
	
    RelocatedMemRegion()
	    : MemRegion(), mapped_base(0)
	{ }
};
