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

/**
 * @brief Memory region data structures
 */

#include <cstdint>
#include <cstddef>
#include <cassert>

typedef unsigned long Address; // address type

/**
 * @brief Arbitrary memory region
 **/
struct MemRegion
{
	MemRegion()
		: base(0), size(0)
	{ }

	MemRegion(Address _base, ptrdiff_t _size)
		: base(_base), size(_size)
	{ }

	virtual ~MemRegion() { }
	
	/**
	 * @brief Region base address
	 **/
	Address   base;
	
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
	bool contains(Address const a) const
	{
		return (this->base <= a) && (a <= (this->base + this->size));
	}

	bool operator==(MemRegion const & other)
	{
		return (base == other.base) and (size == other.size);
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
	Address   mappedBase;
		
	/**
	 * @brief Translate a relocated address to a region address.
	 *
	 * @param a Address within [mapped_base, mapped_base+size]
	 * @return address within [base, base+size]
	 **/
	Address relocToRegion(Address const & a) const
	{
		assert(mappedBase <= a);
		assert(a <= mappedBase + size);
		return a - mappedBase + base;
	}
	
	/**
	 * @brief Translate an unrelocated address to a relocated one.
	 *
	 * @param a Address within [base, base+size]
	 * @return address within [mapped_base, mapped_base+size]
	 **/
	Address regionToReloc(Address const & a) const
	{
		assert(base <= a);
		assert(a <= base + size);
		return a - base + mappedBase;
	}
	
	/**
	 * @brief Determine if the given address is within the relocated region.
	 *
	 * @param a address
	 * @return true if a is witihn [mapped_base, mapped_base+size], false otherwise
	 **/
	bool relocContains(Address const a) const
	{
		return contains(relocToRegion(a));
	}
	
    RelocatedMemRegion()
	    : MemRegion(), mappedBase(0)
	{ }
	
	RelocatedMemRegion (Address base, ptrdiff_t size, Address mapped = 0)
		: MemRegion(base, size), mappedBase(mapped)
	{ }

	RelocatedMemRegion(MemRegion const &m, Address mapped = 0)
		: MemRegion(m.base, m.size), mappedBase(mapped)
	{ }
};
