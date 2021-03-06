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

#include "data/memory.h"
#include "platform/platform.h"

enum class AccessDirection { IN, OUT, INOUT, NONE };

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
	Address ip()               { return _ip; }
	virtual void ip(Address a) { _ip = a; }


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

	/**
	 * @brief Determine if the instruction is a branch instruction.
	 *
	 * @return bool
	 **/
	virtual bool isBranch() = 0;

	/**
	 * @brief Determine if the instruction is a function call
	 *
	 * @return bool
	 **/
	virtual bool isCall() = 0;

	/**
	 * @brief Types of branches
	 **/
	enum BranchType {
		BT_NONE,
		BT_JUMP_UNCOND,
		BT_JUMP_COND,
		BT_CALL,         // call with immediate target
		BT_CALL_DYN,     // call into dynamic library (via PLT/GOT)
		BT_CALL_RESOLVE, // call yet to be resolved (through memory or register)
		BT_JMP_RESOLVE,  // jump yet to be resolved
		BT_RET,
		BT_INT,
	};


	/**
	 * @brief Determine branch targets and type
	 *
	 * @param v vector to be filled with target addresses
	 * @return :BranchType
	 **/
	virtual BranchType branchTargets(std::vector<Address>& v) = 0;

	/**
	 * @brief Determine brancht target belonging to instruction
	 * 
	 * @return :BranchType
	 **/
	virtual Instruction::BranchType opcodeToBranchType() = 0;


	/*
	 * pair(Register ID, access size)
	 */
	typedef std::pair<int, int> RegisterAccessInfo;

	/**
	 * @brief Obtain information about read and written registers
	 *
	 * @param readSet ...
	 * @param writeSet ...
	 * @return void
	 **/
	virtual void getRegisterRWInfo(std::vector<RegisterAccessInfo>& readSet,
	                               std::vector<RegisterAccessInfo>& writeSet) = 0;

protected:
	Address _ip;   // corresponding instruction pointer
	Address _base; // where instruction bytes are in memory
};

