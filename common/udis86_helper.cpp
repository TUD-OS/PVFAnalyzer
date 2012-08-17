#include "instruction/instruction_udis86.h"

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

/*
 * export of Udis86Instruction class ID for serialization
 */
//BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");