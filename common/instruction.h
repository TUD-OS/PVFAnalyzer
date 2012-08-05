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

#include <iostream>
#include <iomanip>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/graph/graph_concepts.hpp>

#include "memory.h"

/**
 * @brief Generic interface for Instructions.
 **/
class Instruction
{
public:
	Instruction() : _ip(0), _base(0) { }
	virtual ~Instruction() { }
	
	/**
	 * @brief Get the length of the instruction in bytes.
	 *
	 * @return unsigned int
	 **/
	virtual unsigned length() = 0;

	/**
	 * @brief Print the instruction
	 *
	 * @return void
	 **/
	virtual void     print()  = 0;
	
	/**
	 * @brief Obtain the instruction's address
	 * 
	 * This is the value of the instruction pointer register
	 * at runtime when executing this instruction.
	 *
	 * @return Address
	 **/
	Address ip()                    { return _ip; }
	virtual void    ip(Address a)   { _ip = a; }


	/**
	 * @brief Get the C-string representation of this instruction
	 *
	 * @return unsigned int
	 **/
	virtual char const *c_str() const = 0;


	/**
	 * @brief Address of instruction bytes
	 * 
	 * This is the instruction's address _right now_
	 * during decoding (and may thus be different from
	 * the RIP value).
	 *
	 * @return Address
	 **/
	Address membase()               { return _base; }
	virtual void membase(Address a) { _base = a; }

	/**
	 * @brief Serialize generic instruction object
	 *
	 * @param a       Boost::Serialization archive
	 * @param version serialization version
	 * @return void
	 **/
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		(void)version;
		ar & _ip;
		ar & _base;
	}
	
protected:
	Address _ip;   // corresponding instruction pointer
	Address _base; // where instruction bytes are in memory
};
