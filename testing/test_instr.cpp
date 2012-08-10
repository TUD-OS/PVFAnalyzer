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

#include "data/memory.h"
#include "data/input.h"

#include "instruction/disassembler.h"

#include "wvtest.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <iostream>

static void
checkSequentialInstruction(Udis86Disassembler& dis, Address memloc, unsigned offset,
                           unsigned len, char const *repr, bool branch = false)
{
	Instruction *i = dis.disassemble(offset);
	WVPASS(i);
	WVPASSEQ(i->ip(), memloc + offset);
	WVPASSEQ(i->membase(), dis.buffer().base + offset);
	WVPASSEQ(i->length(), len);
	std::string str(i->c_str());
	boost::trim(str);
	std::cout << "'" << str << "'" << " <-> '" << repr << "'" << std::endl;
	WVPASS(strcmp(str.c_str(), repr) == 0);
	WVPASSEQ(i->isBranch(), branch);
	delete i;
}


static void
inputToDisassembler(char const *data, Udis86Disassembler& dis, HexbyteInputReader& hir,
                    Address codeAddress, unsigned expect_bytes)
{
	std::vector<std::string> in;
	boost::split(in, data, boost::is_any_of(" "));
	WVPASS(in.size() == expect_bytes);

	BOOST_FOREACH(std::string s, in) {
		hir.addData(s.c_str());
	}
	hir.section(0)->relocationAddress(codeAddress);
	WVPASSEQ(hir.section_count(), 1);
	WVPASSEQ(hir.entry(), 0);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, codeAddress);

	dis.buffer(hir.section(0)->getBuffer());
	WVPASSEQ(hir.section(0)->getBuffer().base, dis.buffer().base);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, dis.buffer().mapped_base);
}


WVTEST_MAIN("single disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;

	inputToDisassembler("c3", dis, hir, 0, 1);
	checkSequentialInstruction(dis, 0, 0, 1, "ret", true);

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(1);
	WVPASS(i == 0);
}


WVTEST_MAIN("two instr. disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler(
		"89 44 24 04 "          /* mov [esp+4], eax */
		"c7 04 24 00 00 00 00", /* movl [esp], 0 */
		dis, hir, 0, 11);

	checkSequentialInstruction(dis, 0, 0, 4, "mov [esp+0x4], eax");
	checkSequentialInstruction(dis, 0, 4, 7, "mov dword [esp], 0x0");

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(11);
	WVPASS(i == 0);
}


WVTEST_MAIN("Instruction::relocated")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;

	inputToDisassembler("e8 bf 0a 00 00", /* call 804d454 */
	                    dis, hir, 0x804c990, 5);

	checkSequentialInstruction(dis, 0x804c990, 0, 5, "call dword 0x804d454", true);

	/* relocate to other base address */
	RelocatedMemRegion r = dis.buffer();
	r.mapped_base = 0;

	Udis86Disassembler d2;
	d2.buffer(r);

	checkSequentialInstruction(d2, 0, 0, 5, "call dword 0xac4", true);

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(5);
	WVPASS(i == 0);

	i = d2.disassemble(5);
	std::cout << (void*)i << std::endl;
	WVPASS(i == 0);
}

static void
test_jnz()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("0f 85 16 01 00 00", dis, hir, 0x804c5fc, 6);
	checkSequentialInstruction(dis, 0x804c5fc, 0, 6, "jnz dword 0x804c718", true);
}

static void
test_call()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("e8 7c ff ff ff", dis, hir, 0x804bfdf, 5);
	checkSequentialInstruction(dis, 0x804bfdf, 0, 5, "call dword 0x804bf60", true);
}


static void
test_call2()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("ff d0", dis, hir, 0, 2);
	checkSequentialInstruction(dis, 0, 0, 2, "call eax", true);
}


static void
test_ret()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("c3", dis, hir, 0x804bfdf, 1);
	checkSequentialInstruction(dis, 0x804bfdf, 0, 1, "ret", true);
}


static void
test_jmp()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("ff 25 60 e1 07 08", dis, hir, 0, 6);
	checkSequentialInstruction(dis, 0, 0, 6, "jmp dword [0x807e160]", true);
}

static void
test_nop()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("90", dis, hir, 0, 1);
	checkSequentialInstruction(dis, 0, 0, 1, "nop", false);
}

WVTEST_MAIN("branch checks")
{
	test_jnz();
	test_call();
	test_call2();
	test_ret();
	test_jmp();
	test_nop();
}