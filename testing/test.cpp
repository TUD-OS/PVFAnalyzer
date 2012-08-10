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

WVTEST_MAIN("memregion")
{
	MemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	
	WVPASSEQ(r.base, 0xf000);
	WVPASSEQ(r.size, 0x0fff);
	WVPASS(r.contains(r.base));
	WVPASS(r.contains(r.base + r.size));
	WVPASS(r.contains(r.base + 5));
	WVFAIL(r.contains(r.base-1));
	WVFAIL(r.contains(r.base + r.size + 1));
}


WVTEST_MAIN("reloc memregion")
{
	RelocatedMemRegion r;
	r.base = 0xF000;
	r.size = 0x0FFF;
	r.mapped_base = 0x10000;
	
	WVPASSEQ(r.base, 0xf000);
	WVPASSEQ(r.size, 0x0fff);
	WVPASSEQ(r.mapped_base, 0x10000);

	WVPASS(r.contains(r.base));
	WVPASS(r.contains(r.base + r.size));
	WVPASS(r.contains(r.base + 5));
	WVFAIL(r.contains(r.base-1));
	WVFAIL(r.contains(r.base + r.size + 1));
	
	WVPASS(r.reloc_contains(0x10200));
	WVPASSEQ(r.region_to_reloc(0xF200),  0x10200);
	WVPASSEQ(r.reloc_to_region(0x10200), 0xF200);
}


WVTEST_MAIN("hex input reader")
{
	HexbyteInputReader ir;
	HexbyteInputReader ir2;
	char const *in[]  =  {"12", "34", "56", "78", "90", "de", "ad", "be", "ef"};
	char const *in2[] =  {"0f", "f0", "ba", "xy", "12"};

	for (unsigned i = 0; i < 9; ++i) {
		ir.addData(in[i]);
	}
	WVPASSEQ(ir.section_count(), 1);
	WVPASSEQ(static_cast<int>(ir.section(0)->bytes()), 9);

	for (unsigned i = 0; i < 5; ++i) {
		ir2.addData(in2[i]);
	}
	WVPASSEQ(ir.section_count(), 1);
	WVPASSEQ(static_cast<int>(ir2.section(0)->bytes()), 4);

	uint8_t const * const ptr = reinterpret_cast<uint8_t const * const>(ir.section(0)->getBuffer().base) + 5;
	WVPASSEQ(*(ptr  ), 0xde);
	WVPASSEQ(*(ptr+1), 0xad);
	WVPASSEQ(*(ptr+2), 0xbe);
	WVPASSEQ(*(ptr+3), 0xef);
}


WVTEST_MAIN("hex input, large")
{
	HexbyteInputReader hr;
	char const *in[] = {"c3"};
	for (unsigned i = 0; i < 3000; ++i) {
		hr.addData(in[0]);
	}
	WVPASSEQ(hr.section_count(), 1);
	WVPASSEQ(hr.entry(), 0);
	WVPASSEQ(static_cast<int>(hr.section(0)->bytes()), 3000);
}

WVTEST_MAIN("file input reader")
{
	char const *file = "testing/testcases/payload.bin";
	FileInputReader fr;
	fr.addData(file);
	WVPASSEQ(fr.section_count(), 1);
	WVPASSEQ(static_cast<int>(fr.section(0)->bytes()), 32);
}

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


WVTEST_MAIN("single disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	char const *input[] = {"c3"};
	hir.addData(input[0]);

	WVPASSEQ(hir.section_count(), 1);
	WVPASSEQ(hir.entry(), 0);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, 0);

	dis.buffer(hir.section(0)->getBuffer());
	WVPASSEQ(hir.section(0)->getBuffer().base, dis.buffer().base);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, dis.buffer().mapped_base);

	checkSequentialInstruction(dis, 0, 0, 1, "ret", true);

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(1);
	WVPASS(i == 0);
}


WVTEST_MAIN("two instr. disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	char const *input[] = {"89", "44", "24", "04",                    /* mov [esp+4], eax */
	                       "c7", "04", "24", "00", "00", "00", "00"}; /* movl [esp], 0 */
	for (unsigned i = 0; i < 11; ++i) {
		hir.addData(input[i]);
	}
	WVPASSEQ(hir.section_count(), 1);
	WVPASSEQ(hir.entry(), 0);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, 0);

	dis.buffer(hir.section(0)->getBuffer());
	WVPASSEQ(hir.section(0)->getBuffer().base, dis.buffer().base);
	WVPASSEQ(hir.section(0)->getBuffer().mapped_base, dis.buffer().mapped_base);

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
	Address const codeAddress = 0x804c990;
	char const *input = {"e8 bf 0a 00 00"};  /* call 804d454 */
	std::vector<std::string> in;
	boost::split(in, input, boost::is_any_of(" "));
	WVPASS(in.size() == 5);

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

	checkSequentialInstruction(dis, codeAddress, 0, 5, "call dword 0x804d454", true);

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