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
#include <udis86.h>
#include "disassembler.h"

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
	
protected:
	Address _ip;   // corresponding instruction pointer
	Address _base; // where instruction bytes are in memory
};


class Udis86Instruction : public Instruction
{
public:
	Udis86Instruction()
		: _ud_obj()
	{
		ud_init(&_ud_obj);
		ud_set_input_file(&_ud_obj, stdin);
		ud_set_mode(&_ud_obj, 32);
		ud_set_syntax(&_ud_obj, UD_SYN_INTEL);

		ip(_ip);
		membase(_base);
	}
	
	virtual ~Udis86Instruction()
	{ }
	
	virtual unsigned length()
	{
		return ud_insn_len(&_ud_obj);
	}
	
	virtual void ip(Address a)
	{
		Instruction::ip(a);
		ud_set_pc(&_ud_obj, a);
	}

	virtual void membase(Address a)
	{
		Instruction::membase(a);
		/* We hardcode the input buffer size to 32 here.
		 * As an Instruction always represents exactly one
		 * instruction, this should suffice.
		 */
		ud_set_input_buffer(&_ud_obj, reinterpret_cast<uint8_t*>(a), 32);
	}

	virtual void print()
	{
		std::cout << "\033[33m";
		std::cout << std::hex << "0x" << std::setw(8) << Instruction::ip();
		std::cout << "    " << "\033[0m";
		std::cout << ud_insn_asm(&_ud_obj);
	}

	/**
	 * @brief Access the Udis86 disassembler object representing this Instruction
	 *
	 * @return ud_t*
	 **/
	ud_t* ud_obj() { return &_ud_obj; }

protected:
	ud_t _ud_obj;

private:
	Udis86Instruction(const Udis86Instruction&) : _ud_obj() { }
	Udis86Instruction& operator=(Udis86Instruction&) { return *this; }
};