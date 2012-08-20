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

#include "instruction/instruction.h"

#include <boost/serialization/vector.hpp>
#include <boost/foreach.hpp>
#include <vector>

/**
 * @brief Representation of a single basic block.
 **/
struct BasicBlock
{
	std::vector<Instruction*> instructions; // instructions in this BB
	Instruction::BranchType   branchType;   // type of branch that terminates this BB

	BasicBlock()
		: instructions(), branchType(Instruction::BranchType::BT_NONE)
	{ }

	~BasicBlock()
	{
		/*
		 * Releasing a BB means to also release all memory associated
		 * with the contained instructions.
		 */
		BOOST_FOREACH(Instruction* i, instructions) {
			delete i;
		}
	}

	/**
	 * @brief Add a single instruction
	 *
	 * @param i Instruction
	 * @return void
	 **/
	void addInstruction(Instruction* i)
	{
		instructions.push_back(i);
	}

	/**
	 * @brief Add a sequence of instructions
	 *
	 * @param orig instruction list
	 * @return void
	 **/
	void addInstructions(std::vector<Instruction*>& orig)
	{
		instructions.insert(instructions.end(),
		                    orig.begin(), orig.end());
	}

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		(void)version;
		a & instructions;
		a & branchType;
	}

	/**
	 * @brief Get the first instruction's address
	 *
	 * @return Address
	 **/
	Address firstInstruction()
	{
		assert(!instructions.empty());
		return instructions.front()->ip();
	}

	/**
	 * @brief Get the address of this BB's _last_ instruction
	 *
	 * Note, that the last instruction is _not_ the last byte
	 * of this BB.
	 *
	 * @return Address
	 **/
	Address lastInstruction()
	{
		assert(!instructions.empty());
		return instructions.back()->ip();
	}
};

