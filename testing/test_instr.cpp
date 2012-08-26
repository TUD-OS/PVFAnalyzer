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
#include "util.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <iostream>

/*
 * Global configuration emulation
 */
class init
{
	Configuration globalCfg;
public:
	init()
		: globalCfg(true, true)
	{
		Configuration::setConfig(&globalCfg);
	}
};

init _i;

/**
 * @brief Test disassembly of a single instruction
 *
 * @param dis disassembler
 * @param memloc base address of instruction buffer (EIP - offset)
 * @param offset offset of instruction within buffer
 * @param len instruction length in bytes
 * @param repr instruction string representation
 * @param branch is a branch instruction Defaults to false.
 * @param release_mem release Instruction memory within this func or return a pointer to caller Defaults to true.
 * @return Instruction*
 **/
static Instruction *
checkSequentialInstruction(Udis86Disassembler& dis, Address memloc, unsigned offset,
                           unsigned len, char const *repr, bool branch = false, bool release_mem = true)
{
	Instruction *i = dis.disassemble(Address(offset));
	WVPASS(i);
	WVPASSEQ(i->ip().v, memloc.v + offset);
	WVPASSEQ(i->membase().v, dis.buffer().base.v + offset);
	WVPASSEQ(i->length(), len);
	std::string str(i->c_str());
	boost::trim(str);
	std::cout << "'" << str << "'" << " <-> '" << repr << "'" << std::endl;
	WVPASS(strcmp(str.c_str(), repr) == 0);
	WVPASSEQ(i->isBranch(), branch);

	if (release_mem) {
		delete i;
		i = 0;
	}

	return i;
}


/**
 * @brief Read input string into a disassembler using a Hexbyte input reader
 *
 * @param data byte string
 * @param dis disassembler
 * @param hir input reader
 * @param codeAddress code base address
 * @param expect_bytes number of bytes to read
 * @return void
 **/
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
	WVPASSEQ(hir.sectionCount(), 1);
	WVPASSEQ(hir.entry().v, 0);
	WVPASSEQ(hir.section(0)->getBuffer().mappedBase.v, codeAddress.v);

	dis.buffer(hir.section(0)->getBuffer());
	WVPASSEQ(hir.section(0)->getBuffer().base.v, dis.buffer().base.v);
	WVPASSEQ(hir.section(0)->getBuffer().mappedBase.v, dis.buffer().mappedBase.v);
}


WVTEST_MAIN("single disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;

	inputToDisassembler("c3", dis, hir, Address(0), 1);
	checkSequentialInstruction(dis, Address(0), 0, 1, "ret", true);

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(Address(1));
	WVPASS(i == 0);
}


WVTEST_MAIN("two instr. disassembly")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler(
		"89 44 24 04 "          /* mov [esp+4], eax */
		"c7 04 24 00 00 00 00", /* movl [esp], 0 */
		dis, hir, Address(0), 11);

	checkSequentialInstruction(dis, Address(0), 0, 4, "mov [esp+0x4], eax");
	checkSequentialInstruction(dis, Address(0), 4, 7, "mov dword [esp], 0x0");

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(Address(11));
	WVPASS(i == 0);
}


WVTEST_MAIN("Instruction::relocated")
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;

	inputToDisassembler("e8 bf 0a 00 00", /* call 804d454 */
	                    dis, hir, Address(0x804c990), 5);

	checkSequentialInstruction(dis, Address(0x804c990), 0, 5, "call dword 0x804d454", true);

	/* relocate to other base address */
	RelocatedMemRegion r = dis.buffer();
	r.mappedBase = Address(0);

	Udis86Disassembler d2;
	d2.buffer(r);

	checkSequentialInstruction(d2, Address(0), 0, 5, "call dword 0xac4", true);

	/* there is no further instruction...*/
	Instruction *i = dis.disassemble(Address(5));
	WVPASS(i == 0);

	i = d2.disassemble(Address(5));
	std::cout << (void*)i << std::endl;
	WVPASS(i == 0);
}

static void
test_jnz()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("0f 85 16 01 00 00", dis, hir, Address(0x804c5fc), 6);
	checkSequentialInstruction(dis, Address(0x804c5fc), 0, 6, "jnz dword 0x804c718", true);
}

static void
test_call()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("e8 7c ff ff ff", dis, hir, Address(0x804bfdf), 5);
	checkSequentialInstruction(dis, Address(0x804bfdf), 0, 5, "call dword 0x804bf60", true);
}


static void
test_call2()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("ff d0", dis, hir, Address(0), 2);
	checkSequentialInstruction(dis, Address(0), 0, 2, "call eax", true);
}


static void
test_ret()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("c3", dis, hir, Address(0x804bfdf), 1);
	checkSequentialInstruction(dis, Address(0x804bfdf), 0, 1, "ret", true);
}


static void
test_jmp()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("ff 25 60 e1 07 08", dis, hir, Address(0), 6);
	checkSequentialInstruction(dis, Address(0), 0, 6, "jmp dword [0x807e160]", true);
}

static void
test_nop()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("90", dis, hir, Address(0), 1);
	checkSequentialInstruction(dis, Address(0), 0, 1, "nop", false);
}

static void
test_int80()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("cd 80", dis, hir, Address(0), 2);
	checkSequentialInstruction(dis, Address(0), 0, 2, "int 0x80", true);
}

WVTEST_MAIN("branch checks")
{
	test_jnz();
	test_call();
	test_call2();
	test_ret();
	test_jmp();
	test_nop();
	test_int80();
}


static void test_int80branch()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("cd 80", dis, hir, Address(0), 2);
	Instruction *i = checkSequentialInstruction(dis, Address(0), 0, 2, "int 0x80", true, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 1);
	WVPASSEQ(btargets[0].v, 2);
	delete i;
}


static void
test_nop_branch()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("90", dis, hir, Address(0), 1);
	Instruction* i = checkSequentialInstruction(dis, Address(0), 0, 1, "nop", false, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 0);
	delete i;
}

static void
test_call_branch()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("e8 dd 20 00 00", dis, hir, Address(0x804fd5a), 5);
	Instruction* i = checkSequentialInstruction(dis, Address(0x804fd5a), 0, 5, "call dword 0x8051e3c", true, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 1);
	WVPASSEQ(btargets[0].v, 0x8051e3c);
	delete i;
}


static void
test_direct_jmp()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("e9 a2 02 00 00", dis, hir, Address(0x80512d4), 5);
	Instruction* i = checkSequentialInstruction(dis, Address(0x80512d4), 0, 5, "jmp dword 0x805157b", true, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 1);
	WVPASSEQ(btargets[0].v, 0x805157b);
	delete i;
}


static void
test_condjmp_branch()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("75 02", dis, hir, Address(0x80515aa), 2);
	Instruction* i = checkSequentialInstruction(dis, Address(0x80515aa), 0, 2, "jnz 0x80515ae", true, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 2);
	WVPASSEQ(btargets[0].v, 0x80515ae);
	WVPASSEQ(btargets[1].v, 0x80515ac);
	delete i;
}

static void
test_condjmp_neg()
{
	Udis86Disassembler dis;
	HexbyteInputReader hir;
	inputToDisassembler("7e fa", dis, hir, Address(0x100), 2);
	Instruction* i = checkSequentialInstruction(dis, Address(0x100), 0, 2, "jle 0xfc", true, false);
	WVPASS(i != 0);
	std::vector<Address> btargets;
	i->branchTargets(btargets);
	WVPASS(btargets.size() == 2);
	WVPASSEQ(btargets[0].v, 0xfc);
	WVPASSEQ(btargets[1].v, 0x102);
	delete i;
}

WVTEST_MAIN("branch targets")
{
	test_int80branch();
	test_nop_branch();
	test_call_branch();
	test_direct_jmp();
	test_condjmp_branch();
	test_condjmp_neg();
}
