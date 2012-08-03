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

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/graph/graph_concepts.hpp>

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


struct udis86_t : public ud_t
{
	udis86_t()
		: ud_t()
	{
		ud_init(udptr());
		ud_set_mode(udptr(), 32);
		ud_set_syntax(udptr(), UD_SYN_INTEL);
	}

	ud_t* udptr() { return reinterpret_cast<ud_t*>(this); }
	ud_t const *udp() const { return reinterpret_cast<ud_t const *>(this); }

	/**
	 * @brief Serialize those parts of ud_t that are relevant for us.
	 *
	 * Note: We assume that we serialize/deserialize single disassembled
	 *       instructions here. This means that the instruction has already
	 *       been successfully disassembled using ud_disassemble(). Hence we
	 *       can ignore the corresponding input buffer.
	 *
	 * @param a serialization archive
	 * @param version version
	 * @return void
	 **/
	template <class Archive>
	void serialize(Archive& a, const unsigned int version)
	{
		/* Collecting some info about ud_t components here.
		 *
		 * -> inp* are only used during disassembly -> we don't store them
		 *    as we assume an already disassembled instruction
		 *
		 */
		std::cerr << "udis86_t serialization is unimplemented!" << std::endl;
	}
};

class Udis86Instruction : public Instruction
{
public:
	Udis86Instruction()
		: Instruction(), _ud_obj()
	{
		ud_set_input_file(_ud_obj.udptr(), stdin);

		ip(_ip);
		membase(_base);
	}
	
	virtual ~Udis86Instruction()
	{ }
	
	virtual unsigned length()
	{
		return ud_insn_len(_ud_obj.udptr());
	}
	
	virtual void ip(Address a)
	{
		Instruction::ip(a);
		ud_set_pc(_ud_obj.udptr(), a);
	}

	virtual void membase(Address a)
	{
		Instruction::membase(a);
		/* We hardcode the input buffer size to 32 here.
		 * As an Instruction always represents exactly one
		 * instruction, this should suffice.
		 */
		ud_set_input_buffer(_ud_obj.udptr(), reinterpret_cast<uint8_t*>(a), 32);
	}

	virtual void print()
	{
		std::cout << "\033[33m";
		std::cout << std::hex << "0x" << std::setfill('0') << std::setw(8) << Instruction::ip();
		std::cout << "    " << "\033[0m";
		std::cout << c_str();
	}


	virtual char const *c_str() const
	{
		return ud_insn_asm(const_cast<ud*>(_ud_obj.udp()));
	}


	/**
	 * @brief Serialize/Deserialize UDis86 instruction
	 **/
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		boost::serialization::base_object<Instruction>(*this);
		ar & _ud_obj;
	}

	ud* ud_obj() { return _ud_obj.udptr(); }

protected:
	udis86_t _ud_obj;

private:
	Udis86Instruction(const Udis86Instruction&) : _ud_obj() { }
	Udis86Instruction& operator=(Udis86Instruction&) { return *this; }
};
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
