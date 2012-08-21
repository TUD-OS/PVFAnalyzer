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
#include "util.h"

#include <udis86.h>
#include <cstdio>


/**
 * @brief Wrapper struct for udis86's data structure
 *
 * ud_t contains ud_operand members that need to have dedicated
 * serialization routine as well.
 **/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
struct udop : public ud_operand_t
{
	udop(ud_operand_t& op)
	{
		*this = op;
	}

	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		ar & size;
		ar & lval.uqword;
		ar & base;
		ar & index;
		ar & offset;
		ar & scale;
	}
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

	ud_t* udptr()           { return reinterpret_cast<ud_t*>(this); }
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
	void serialize(Archive& ar, const unsigned int version)
	{
		/* Collecting some info about ud_t components here.
		 *
		 * -> inp* are only used during disassembly -> we don't store them
		 *    as we assume an already disassembled instruction
		 *
		 * Exception: inp_ctr is used to store the length of the instruction!
		 */

		ar & inp_ctr;

		ar & insn_offset;
		ar & insn_hexcode;
		ar & insn_buffer;
		ar & insn_fill;
		ar & dis_mode;
		ar & pc;
		ar & vendor;

		ar & mnemonic;
		ar & static_cast<udop&> ( operand[0] );
		ar & static_cast<udop&> ( operand[1] );
		ar & static_cast<udop&> ( operand[2] );

		ar & error;
		ar & pfx_rex;
		ar & pfx_seg;
		ar & pfx_opr;
		ar & pfx_adr;
		ar & pfx_lock;
		ar & pfx_rep;
		ar & pfx_repe;
		ar & pfx_repne;
		ar & pfx_insn;
		ar & default64;
		ar & opr_mode;
		ar & adr_mode;
		ar & br_far;
		ar & br_near;
		ar & implicit_addr;
		ar & c1;
		ar & c2;
		ar & c3;
		ar & have_modrm;
		ar & modrm;
	}
};
#pragma GCC diagnostic pop

/**
 * @brief Utility helper for interfacing with UDIS86
 **/
class Udis86Helper
{
public:
	/**
	 * @brief Print UDIS86 opcode
	 *
	 * @param op opcode / operand type / ...
	 * @return void
	 **/
	static void    printUDOp(unsigned op);

	
	/**
	 * @brief Get the numeric value of a given operand
	 *
	 * @param ud Udis86 instruction
	 * @param operandNo operand number (0-2)
	 * @return int64_t
	 **/
	static int64_t operandToValue(ud_t *ud, unsigned operandNo);

	static unsigned operandCount(ud_t *ud);
	static bool     modifiesOperand(ud_t *ud, unsigned num);
};

/**
 * @brief Udis86-specific instruction information
 **/
class Udis86Instruction : public Instruction
{
public:
	Udis86Instruction()
		: Instruction(), _udObj()
	{
		ud_set_input_file(_udObj.udptr(), stdin);

		ip(_ip);
		membase(_base);
	}
	
	virtual ~Udis86Instruction()
	{ }
	
	/**
	 * @brief Determine the instruction's length in bytes.
	 *
	 * @return unsigned int
	 **/
	virtual unsigned length()
	{
		return ud_insn_len(_udObj.udptr());
	}
	
	/**
	 * @brief Set the instruction's address
	 *
	 * @param a EIP
	 * @return void
	 **/
	virtual void ip(Address a)
	{
		Instruction::ip(a);
		ud_set_pc(_udObj.udptr(), a);
	}

	/**
	 * @brief Set the byte stream this instruction belongs to
	 *
	 * @param a buffer address
	 * @return void
	 **/
	virtual void membase(Address a)
	{
		Instruction::membase(a);
		/* We hardcode the input buffer size to 32 here.
		 * As an Instruction always represents exactly one
		 * instruction, this should suffice.
		 */
		ud_set_input_buffer(_udObj.udptr(), reinterpret_cast<uint8_t*>(a), 32);
	}

	/**
	 * @brief Print the instruction to std::cout
	 *
	 * @return void
	 **/
	virtual void print()
	{
		std::cout << "\033[33m";
		std::cout << std::hex << "0x" << std::setfill('0') << std::setw(8) << Instruction::ip();
		std::cout << "    " << "\033[0m";
		std::cout << c_str();
	}


	/**
	 * @brief Obtain a C-string representation of this instruction.
	 *
	 * @return char*
	 **/
	virtual char const *c_str() const
	{
		return ud_insn_asm(const_cast<ud*>(_udObj.udp()));
	}


	/**
	 * @brief Serialize/Deserialize UDis86 instruction
	 **/
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & boost::serialization::base_object<Instruction>(*this);
		ar & _udObj;
	}

	ud* udObj() { return _udObj.udptr(); }

	virtual bool isBranch()
	{
		switch(udObj()->mnemonic) {
			case UD_Ija:		case UD_Ijae:	case UD_Ijb:
			case UD_Ijbe:	case UD_Ijcxz:	case UD_Ijecxz:
			case UD_Ijg:		case UD_Ijge:	case UD_Ijl:
			case UD_Ijle:	case UD_Ijmp:	case UD_Ijno:
			case UD_Ijnp:	case UD_Ijns:	case UD_Ijnz:
			case UD_Ijo:		case UD_Ijp:		case UD_Ijs:
			case UD_Ijz:		case UD_Ijrcxz:	case UD_Icall:
			case UD_Iret:	case UD_Iint:
			/* yep, syscalls branch to somewhere else, too */
			case UD_Isyscall: case UD_Isysenter: case UD_Isysexit:
			case UD_Isysret:
				return true;
			default: break;
		}
		return false;
	}


	virtual Instruction::BranchType branchTargets(std::vector<Address>& targets)
	{
		if (!isBranch())
			return BT_NONE;

		ud_t *ud = udObj();
		DEBUG(std::cout << "jump: " << ud_insn_asm(ud) << std::endl;);
		/* Assumption: jumps always have a single target. */
		assert(ud->operand[1].type == UD_NONE);
		assert(ud->operand[2].type == UD_NONE);

		int target;

		switch(ud->operand[0].type) {
			case UD_OP_IMM:  /* Immediate operand. Value available in lval. */
				/* The immediate operand for INT xx is not a jump target. */
				if (ud->mnemonic != UD_Iint) { // XXX is INT xx the special or the common case?
					target  = Udis86Helper::operandToValue(ud, 0);
					//DEBUG(std::cout << "branch to: " << target << std::endl;);
					targets.push_back(Udis86Helper::operandToValue(ud, 0));
				}
				break;
			case UD_OP_JIMM: /* Immediate operand to branch instruction (relative offsets).
			                    Value available in lval */
				target  = Udis86Helper::operandToValue(ud, 0);
#if 0
				DEBUG(std::cout << "branch to: " << Instruction::ip() << "+" << this->length()
				                << "+" << std::dec << target << "="
				                << std::hex << Instruction::ip() + length() + target
				                << std::endl;);
#endif
				target += Instruction::ip();
				target += length();
				targets.push_back(target);
				break;
			case UD_NONE:
				//DEBUG(std::cout << "BRANCH to UD_NONE." << std::endl;);
				break;
			default:
				DEBUG(std::cout << ud->operand[0].type << std::endl;);
				throw NotImplementedException("branch target type");
		}

		switch(ud->mnemonic) {
			/* The following instructions also have their successor instruction
			 * as branch target: */
			case UD_Ija:		case UD_Ijae:	case UD_Ijb:
			case UD_Ijbe:	case UD_Ijcxz:	case UD_Ijecxz:
			case UD_Ijg:		case UD_Ijge:	case UD_Ijl:
			case UD_Ijle:	case UD_Ijno:
			case UD_Ijnp:	case UD_Ijns:	case UD_Ijnz:
			case UD_Ijo:		case UD_Ijp:		case UD_Ijs:
			case UD_Ijz:		case UD_Ijrcxz:	case UD_Iint:
				targets.push_back(Instruction::ip() + length());
				break;
			default:
				break;
		}

		BranchType bt = BT_NONE;
		/* Branch type detection */
		switch(ud->mnemonic) {
			case UD_Ija:		case UD_Ijae:	case UD_Ijb:		case UD_Ijbe:
			case UD_Ijcxz:	case UD_Ijecxz:	case UD_Ijg:		case UD_Ijge:
			case UD_Ijl:		case UD_Ijle:	case UD_Ijno:	case UD_Ijnp:
			case UD_Ijns:	case UD_Ijnz:	case UD_Ijo:		case UD_Ijp:
			case UD_Ijs:		case UD_Ijz:		case UD_Ijrcxz:
				bt = BT_JUMP_COND;
				//DEBUG(std::cout << "BT_JUMP_COND" << std::endl;);
				break;
			case UD_Icall:
				bt = BT_CALL;
				//DEBUG(std::cout << "BT_CALL" << std::endl;);
				break;
			case UD_Iret:
				bt = BT_RET;
				//DEBUG(std::cout << "BT_RET" << std::endl;);
				break;
			case UD_Iint:
				bt = BT_INT;
				//DEBUG(std::cout << "BT_INT" << std::endl;);
				break;
			case UD_Ijmp:
				bt = BT_JUMP_UNCOND;
				//DEBUG(std::cout << "BT_JUMP_UNCOND" << std::endl;);
				break;
			/* yep, syscalls branch to somewhere else, too */
			case UD_Isyscall: case UD_Isysenter: case UD_Isysexit:
			case UD_Isysret:
				bt = BT_INT;
				break;
			default:
				break;
		}

		return bt;
	}

	virtual void getRegisterRWInfo(std::vector<RegisterAccessInfo>& readSet,
	                               std::vector<RegisterAccessInfo>& writeSet);

protected:
	udis86_t _udObj;

private:
	Udis86Instruction(const Udis86Instruction&) : _udObj() { }
	Udis86Instruction& operator=(Udis86Instruction&) { return *this; }

	void adjustFalsePositives(std::vector<RegisterAccessInfo>& readSet,
	                          std::vector<RegisterAccessInfo>& writeSet);
	void fillAccessInfo(unsigned opno, std::vector<RegisterAccessInfo>& vec);
};
