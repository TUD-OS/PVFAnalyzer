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
#include <vector>

struct BasicBlock
{
	std::vector<Instruction*> instructions;

	BasicBlock()
		: instructions()
	{ }

	/**
	 * @brief Add a single instruction
	 *
	 * @param i Instruction
	 * @return void
	 **/
	void add_instruction(Instruction* i)
	{
		instructions.push_back(i);
	}

	/**
	 * @brief Add a sequence of instructions
	 *
	 * @param orig instruction list
	 * @return void
	 **/
	void add_instructions(std::vector<Instruction*>& orig)
	{
		instructions.insert(instructions.end(),
							orig.begin(), orig.end());
	}

	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		a & instructions;
	}
};