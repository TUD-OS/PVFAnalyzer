#include "disassembler.h"

void Udis86Disassembler::disassemble(const uint8_t* buf)
{

}

Udis86Disassembler::Udis86Disassembler()
	: _ud_obj()
{
	ud_init(&_ud_obj);
	ud_set_input_file(&_ud_obj, stdin);
	ud_set_mode(&_ud_obj, 32);
	ud_set_syntax(&_ud_obj, UD_SYN_INTEL);
}