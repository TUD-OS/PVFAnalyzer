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

#include "instruction/instruction_udis86.h"

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <sstream>
#include <string>
#include <map>

void Udis86Helper::printUDOp(unsigned op)
{
	switch(op) {
		case UD_OP_CONST:
			std::cout << "Constant";
			break;
		case UD_OP_IMM:
			std::cout << "Immediate";
			break;
		case UD_OP_JIMM:
			std::cout << "J-Immediate (\?\?\?)";
			break;
		case UD_OP_MEM:
			std::cout << "Memory";
			break;
		case UD_OP_PTR:
			std::cout << "pointer";
			break;
		case UD_OP_REG:
			std::cout << "register";
			break;
		default:
			std::cout << "Unknown operand: " << op;
	}
}


int64_t Udis86Helper::operandToValue(ud_t *ud, unsigned opno)
{
	ud_operand_t op = ud->operand[opno];
	switch(op.type) {
		case UD_OP_CONST:    /* Values are immediately available in lval */
		case UD_OP_IMM:
		case UD_OP_JIMM:
			//DEBUG(std::cout << op.lval.sqword << std::endl;);
			break;
		default:
			DEBUG(std::cout << op.type << std::endl;);
			throw NotImplementedException("operand to value");
	}

	int64_t ret = op.lval.sqword;

	switch(op.size) {
		case  8:
			ret &= 0xFF;
			if (ret & 0x80) { ret -= 0x100; }
			break;
		case 16:
			ret &= 0xFFFF;
			if (ret & 0x8000) { ret -= 0x10000; }
			break;
		case 32:
			ret &= 0xFFFFFFFF;
			if (ret & 0x80000000) { ret-=0x100000000; }
			break;
		case 64:
			throw NotImplementedException("64 bit operand to value");
			break;
		default:
			throw NotImplementedException("Invalid udis86 operand size");
	}

	return ret;
}


unsigned Udis86Helper::operandCount(ud_t *ud)
{
	unsigned ret = 0;
	for (unsigned i = 0; i < 3; ++i, ++ret) {
		if (ud->operand[i].type == UD_NONE) {
			break;
		}
	}
	return ret;
}

typedef boost::tuple<bool, bool, bool> ModificationInfo;
typedef std::map<unsigned, ModificationInfo> OpcodeModificationMap;

static void initOpcodeModMap(OpcodeModificationMap& m)
{
/* macros for the two common cases */
#define NO_MODIFICATION(x) \
	m[x] = ModificationInfo(false, false, false)

#define MODIFY_SINGLE(x) \
	m[x] = ModificationInfo(true, false, false);

	MODIFY_SINGLE(UD_Iadc);
	MODIFY_SINGLE(UD_Iadd);
	MODIFY_SINGLE(UD_Iand);
	NO_MODIFICATION(UD_Icall);
	NO_MODIFICATION(UD_Icmp);
	NO_MODIFICATION(UD_Iint);
	NO_MODIFICATION(UD_Iint1);
	NO_MODIFICATION(UD_Iint3);
	NO_MODIFICATION(UD_Ijmp);
	NO_MODIFICATION(UD_Ija);
	NO_MODIFICATION(UD_Ijae);
	NO_MODIFICATION(UD_Ijb);
	NO_MODIFICATION(UD_Ijbe);
	NO_MODIFICATION(UD_Ijg);
	NO_MODIFICATION(UD_Ijge);
	NO_MODIFICATION(UD_Ijle);
	NO_MODIFICATION(UD_Ijno);
	NO_MODIFICATION(UD_Ijnp);
	NO_MODIFICATION(UD_Ijns);
	NO_MODIFICATION(UD_Ijnz);
	NO_MODIFICATION(UD_Ijs);
	NO_MODIFICATION(UD_Ijz);
	MODIFY_SINGLE(UD_Ilea);
	MODIFY_SINGLE(UD_Imov);
	MODIFY_SINGLE(UD_Imovzx);
	MODIFY_SINGLE(UD_Ior);
	MODIFY_SINGLE(UD_Ipop);
	NO_MODIFICATION(UD_Ipush);
	MODIFY_SINGLE(UD_Isetnz);
	MODIFY_SINGLE(UD_Ishl);
	MODIFY_SINGLE(UD_Ishld);
	MODIFY_SINGLE(UD_Ishr);
	MODIFY_SINGLE(UD_Ishrd);
	MODIFY_SINGLE(UD_Isub);
	NO_MODIFICATION(UD_Itest);
	MODIFY_SINGLE(UD_Ixor);

#undef NO_MODIFICATION
#undef MODIFY_SINGLE
}

bool Udis86Helper::modifiesOperand(ud_t* ud, unsigned opno)
{
	static OpcodeModificationMap opcodeModifyTable;
	static bool tableInitialized = false;

	assert(opno <= 2);

	if (!tableInitialized) {
		initOpcodeModMap(opcodeModifyTable);
		tableInitialized = true;
	}

	OpcodeModificationMap::const_iterator it = opcodeModifyTable.find(ud->mnemonic);
	if (it == opcodeModifyTable.end()) {
		std::stringstream str;
		str << __func__ << ": Unknown mnemonic " << boost::lexical_cast<std::string>(ud->mnemonic) << " :: " << ud_insn_asm(ud);
		std::string msg(str.str());
		throw NotImplementedException(msg.c_str());
	}

	switch(opno) {
		case 0: return (it->second).get<0>();
		case 1: return (it->second).get<1>();
		case 2: return (it->second).get<2>();
		default:
			throw ThisShouldNeverHappenException(__func__);
	}
}


void Udis86Instruction::fillAccessInfo(unsigned opno, std::vector<RegisterAccessInfo>& set)
{
	ud_t *ud = udObj();
	unsigned reg, size;

	switch (ud->operand[opno].type) {
		case UD_OP_REG:
		{
			reg  = PlatformX8632::UdisToPlatformRegister(ud->operand[opno].base);
			size = ud->operand[opno].size;
			if (!size)
				size = PlatformX8632::UdisToPlatformAccessSize(ud->operand[opno].base);
			//DEBUG(std::cout << reg << ", " << size << std::endl;);
			set.push_back(RegisterAccessInfo(reg, size));
			return;
		}

		case UD_OP_MEM:
		{
			/*
			 * Address may be calculated from a base and an index register (plus a scale, but this
			 * is a constant, which we don't care about.
			 */
			if (ud->operand[opno].base != UD_NONE) {
				reg = PlatformX8632::UdisToPlatformRegister(ud->operand[opno].base);
				set.push_back(RegisterAccessInfo(reg, 32));
			}
			if (ud->operand[opno].index != UD_NONE) {
				reg = PlatformX8632::UdisToPlatformRegister(ud->operand[opno].index);
				set.push_back(RegisterAccessInfo(reg, 32));
			}
			return;
		}

		case UD_OP_IMM:
		case UD_OP_CONST:
		case UD_NONE:
		case UD_OP_JIMM:
			/*
			 * Nothing to do here.
			 */
			return;

		default:
			std::cout << "Handle type: " << ud->operand[opno].type << std::endl;
			break;
	}
	throw NotImplementedException(__func__);
}


void Udis86Instruction::getRegisterRWInfo(std::vector<RegisterAccessInfo>& readSet,
                                          std::vector<RegisterAccessInfo>& writeSet)
{
	ud_t *ud = udObj();

	unsigned numOperands = Udis86Helper::operandCount(ud);
	//DEBUG(std::cout << "      " << numOperands << " operand(s)." << std::endl;);

	for (unsigned i = 0; i < numOperands; ++i) {
		if (Udis86Helper::modifiesOperand(ud, i)) {
			/*
			 * Special handling: If a modified operand has the MEM type,
			 * we still fill into the readSet, because the registers are
			 * only _read_ during calculation of the address.
			 */
			if (ud->operand[i].type == UD_OP_MEM) {
				this->fillAccessInfo(i, readSet);
			} else {
				this->fillAccessInfo(i, writeSet);
			}
		} else {
			this->fillAccessInfo(i, readSet);
		}
	}

	adjustFalsePositives(readSet, writeSet);
}

void Udis86Instruction::adjustFalsePositives(std::vector<RegisterAccessInfo>& readSet,
                                             std::vector<RegisterAccessInfo>& writeSet)
{
	ud_t *ud = udObj();
	if ((ud->mnemonic == UD_Ixor) and                 // is an XOR?
		(ud->operand[0].type == UD_OP_REG) and (ud->operand[1].type == UD_OP_REG) and // both operands are REG
		(ud->operand[0].base == ud->operand[1].base)) // operands are identical
	{
		readSet.clear(); // hard-coded. We only have 2 operands here. Simply remove the read.
	}
}
