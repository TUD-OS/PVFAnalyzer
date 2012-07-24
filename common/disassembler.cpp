#include "disassembler.h"
#include <iostream>

unsigned Udis86Disassembler::disassemble(const uint8_t* buf)
{
	uint8_t *b = (uint8_t*)buf; // XXX: evil cast! But udis86 wants a non-const
	                            //      pointer here...
	ud_set_input_buffer(&_ud_obj, b, 16);
	ud_disassemble(&_ud_obj);
	std::cout << ud_insn_asm(&_ud_obj) << std::endl;

	return ud_insn_len(&_ud_obj);
}

Udis86Disassembler::Udis86Disassembler()
	: _ud_obj()
{
	ud_init(&_ud_obj);
	ud_set_input_file(&_ud_obj, stdin);
	ud_set_mode(&_ud_obj, 32);
	ud_set_syntax(&_ud_obj, UD_SYN_INTEL);
}