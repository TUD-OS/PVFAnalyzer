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


char const *
Udis86Helper::mnemonicToString(unsigned mnemonic)
{
#define CASE(x) case x: return #x;

	switch(mnemonic) {
		CASE(UD_Iinvalid) CASE(UD_I3dnow) CASE(UD_Inone) CASE(UD_Idb) CASE(UD_Ipause) CASE(UD_Iaaa)
		CASE(UD_Iaad) CASE(UD_Iaam) CASE(UD_Iaas) CASE(UD_Iadc) CASE(UD_Iadd) CASE(UD_Iaddpd)
		CASE(UD_Iaddps) CASE(UD_Iaddsd) CASE(UD_Iaddss) CASE(UD_Iand) CASE(UD_Iandpd) CASE(UD_Iandps)
		CASE(UD_Iandnpd) CASE(UD_Iandnps) CASE(UD_Iarpl) CASE(UD_Imovsxd) CASE(UD_Ibound) CASE(UD_Ibsf)
		CASE(UD_Ibsr) CASE(UD_Ibswap) CASE(UD_Ibt) CASE(UD_Ibtc) CASE(UD_Ibtr) CASE(UD_Ibts)
		CASE(UD_Icall) CASE(UD_Icbw) CASE(UD_Icwde) CASE(UD_Icdqe) CASE(UD_Iclc) CASE(UD_Icld)
		CASE(UD_Iclflush) CASE(UD_Iclgi) CASE(UD_Icli) CASE(UD_Iclts) CASE(UD_Icmc) CASE(UD_Icmovo)
		CASE(UD_Icmovno) CASE(UD_Icmovb) CASE(UD_Icmovae) CASE(UD_Icmovz) CASE(UD_Icmovnz) CASE(UD_Icmovbe)
		CASE(UD_Icmova) CASE(UD_Icmovs) CASE(UD_Icmovns) CASE(UD_Icmovp) CASE(UD_Icmovnp) CASE(UD_Icmovl)
		CASE(UD_Icmovge) CASE(UD_Icmovle) CASE(UD_Icmovg) CASE(UD_Icmp) CASE(UD_Icmppd) CASE(UD_Icmpps)
		CASE(UD_Icmpsb) CASE(UD_Icmpsw) CASE(UD_Icmpsd) CASE(UD_Icmpsq) CASE(UD_Icmpss) CASE(UD_Icmpxchg)
		CASE(UD_Icmpxchg8b) CASE(UD_Icomisd) CASE(UD_Icomiss) CASE(UD_Icpuid) CASE(UD_Icvtdq2pd) CASE(UD_Icvtdq2ps)
		CASE(UD_Icvtpd2dq) CASE(UD_Icvtpd2pi) CASE(UD_Icvtpd2ps) CASE(UD_Icvtpi2ps) CASE(UD_Icvtpi2pd) CASE(UD_Icvtps2dq)
		CASE(UD_Icvtps2pi) CASE(UD_Icvtps2pd) CASE(UD_Icvtsd2si) CASE(UD_Icvtsd2ss) CASE(UD_Icvtsi2ss) CASE(UD_Icvtss2si)
		CASE(UD_Icvtss2sd) CASE(UD_Icvttpd2pi) CASE(UD_Icvttpd2dq) CASE(UD_Icvttps2dq) CASE(UD_Icvttps2pi) CASE(UD_Icvttsd2si)
		CASE(UD_Icvtsi2sd) CASE(UD_Icvttss2si) CASE(UD_Icwd) CASE(UD_Icdq) CASE(UD_Icqo) CASE(UD_Idaa)
		CASE(UD_Idas) CASE(UD_Idec) CASE(UD_Idiv) CASE(UD_Idivpd) CASE(UD_Idivps) CASE(UD_Idivsd)
		CASE(UD_Idivss) CASE(UD_Iemms) CASE(UD_Ienter) CASE(UD_If2xm1) CASE(UD_Ifabs) CASE(UD_Ifadd)
		CASE(UD_Ifaddp) CASE(UD_Ifbld) CASE(UD_Ifbstp) CASE(UD_Ifchs) CASE(UD_Ifclex) CASE(UD_Ifcmovb)
		CASE(UD_Ifcmove) CASE(UD_Ifcmovbe) CASE(UD_Ifcmovu) CASE(UD_Ifcmovnb) CASE(UD_Ifcmovne) CASE(UD_Ifcmovnbe)
		CASE(UD_Ifcmovnu) CASE(UD_Ifucomi) CASE(UD_Ifcom) CASE(UD_Ifcom2) CASE(UD_Ifcomp3) CASE(UD_Ifcomi)
		CASE(UD_Ifucomip) CASE(UD_Ifcomip) CASE(UD_Ifcomp) CASE(UD_Ifcomp5) CASE(UD_Ifcompp) CASE(UD_Ifcos)
		CASE(UD_Ifdecstp) CASE(UD_Ifdiv) CASE(UD_Ifdivp) CASE(UD_Ifdivr) CASE(UD_Ifdivrp) CASE(UD_Ifemms)
		CASE(UD_Iffree) CASE(UD_Iffreep) CASE(UD_Ificom) CASE(UD_Ificomp) CASE(UD_Ifild) CASE(UD_Ifncstp)
		CASE(UD_Ifninit) CASE(UD_Ifiadd) CASE(UD_Ifidivr) CASE(UD_Ifidiv) CASE(UD_Ifisub) CASE(UD_Ifisubr)
		CASE(UD_Ifist) CASE(UD_Ifistp) CASE(UD_Ifisttp) CASE(UD_Ifld) CASE(UD_Ifld1) CASE(UD_Ifldl2t)
		CASE(UD_Ifldl2e) CASE(UD_Ifldlpi) CASE(UD_Ifldlg2) CASE(UD_Ifldln2) CASE(UD_Ifldz) CASE(UD_Ifldcw)
		CASE(UD_Ifldenv) CASE(UD_Ifmul) CASE(UD_Ifmulp) CASE(UD_Ifimul) CASE(UD_Ifnop) CASE(UD_Ifpatan)
		CASE(UD_Ifprem) CASE(UD_Ifprem1) CASE(UD_Ifptan) CASE(UD_Ifrndint) CASE(UD_Ifrstor) CASE(UD_Ifnsave)
		CASE(UD_Ifscale) CASE(UD_Ifsin) CASE(UD_Ifsincos) CASE(UD_Ifsqrt) CASE(UD_Ifstp) CASE(UD_Ifstp1)
		CASE(UD_Ifstp8) CASE(UD_Ifstp9) CASE(UD_Ifst) CASE(UD_Ifnstcw) CASE(UD_Ifnstenv) CASE(UD_Ifnstsw)
		CASE(UD_Ifsub) CASE(UD_Ifsubp) CASE(UD_Ifsubr) CASE(UD_Ifsubrp) CASE(UD_Iftst) CASE(UD_Ifucom)
		CASE(UD_Ifucomp) CASE(UD_Ifucompp) CASE(UD_Ifxam) CASE(UD_Ifxch) CASE(UD_Ifxch4) CASE(UD_Ifxch7)
		CASE(UD_Ifxrstor) CASE(UD_Ifxsave) CASE(UD_Ifpxtract) CASE(UD_Ifyl2x) CASE(UD_Ifyl2xp1) CASE(UD_Ihlt)
		CASE(UD_Iidiv) CASE(UD_Iin) CASE(UD_Iimul) CASE(UD_Iinc) CASE(UD_Iinsb) CASE(UD_Iinsw)
		CASE(UD_Iinsd) CASE(UD_Iint1) CASE(UD_Iint3) CASE(UD_Iint) CASE(UD_Iinto) CASE(UD_Iinvd)
		CASE(UD_Iinvept) CASE(UD_Iinvlpg) CASE(UD_Iinvlpga) CASE(UD_Iinvvpid) CASE(UD_Iiretw) CASE(UD_Iiretd)
		CASE(UD_Iiretq) CASE(UD_Ijo) CASE(UD_Ijno) CASE(UD_Ijb) CASE(UD_Ijae) CASE(UD_Ijz)
		CASE(UD_Ijnz) CASE(UD_Ijbe) CASE(UD_Ija) CASE(UD_Ijs) CASE(UD_Ijns) CASE(UD_Ijp)
		CASE(UD_Ijnp) CASE(UD_Ijl) CASE(UD_Ijge) CASE(UD_Ijle) CASE(UD_Ijg) CASE(UD_Ijcxz)
		CASE(UD_Ijecxz) CASE(UD_Ijrcxz) CASE(UD_Ijmp) CASE(UD_Ilahf) CASE(UD_Ilar) CASE(UD_Ilddqu)
		CASE(UD_Ildmxcsr) CASE(UD_Ilds) CASE(UD_Ilea) CASE(UD_Iles) CASE(UD_Ilfs) CASE(UD_Ilgs)
		CASE(UD_Ilidt) CASE(UD_Ilss) CASE(UD_Ileave) CASE(UD_Ilfence) CASE(UD_Ilgdt) CASE(UD_Illdt)
		CASE(UD_Ilmsw) CASE(UD_Ilock) CASE(UD_Ilodsb) CASE(UD_Ilodsw) CASE(UD_Ilodsd) CASE(UD_Ilodsq)
		CASE(UD_Iloopnz) CASE(UD_Iloope) CASE(UD_Iloop) CASE(UD_Ilsl) CASE(UD_Iltr) CASE(UD_Imaskmovq)
		CASE(UD_Imaxpd) CASE(UD_Imaxps) CASE(UD_Imaxsd) CASE(UD_Imaxss) CASE(UD_Imfence) CASE(UD_Iminpd)
		CASE(UD_Iminps) CASE(UD_Iminsd) CASE(UD_Iminss) CASE(UD_Imonitor) CASE(UD_Imontmul) CASE(UD_Imov)
		CASE(UD_Imovapd) CASE(UD_Imovaps) CASE(UD_Imovd) CASE(UD_Imovhpd) CASE(UD_Imovhps) CASE(UD_Imovlhps)
		CASE(UD_Imovlpd) CASE(UD_Imovlps) CASE(UD_Imovhlps) CASE(UD_Imovmskpd) CASE(UD_Imovmskps) CASE(UD_Imovntdq)
		CASE(UD_Imovnti) CASE(UD_Imovntpd) CASE(UD_Imovntps) CASE(UD_Imovntq) CASE(UD_Imovq) CASE(UD_Imovsb)
		CASE(UD_Imovsw) CASE(UD_Imovsd) CASE(UD_Imovsq) CASE(UD_Imovss) CASE(UD_Imovsx) CASE(UD_Imovupd)
		CASE(UD_Imovups) CASE(UD_Imovzx) CASE(UD_Imul) CASE(UD_Imulpd) CASE(UD_Imulps) CASE(UD_Imulsd)
		CASE(UD_Imulss) CASE(UD_Imwait) CASE(UD_Ineg) CASE(UD_Inop) CASE(UD_Inot) CASE(UD_Ior)
		CASE(UD_Iorpd) CASE(UD_Iorps) CASE(UD_Iout) CASE(UD_Ioutsb) CASE(UD_Ioutsw) CASE(UD_Ioutsd)
		CASE(UD_Ioutsq) CASE(UD_Ipacksswb) CASE(UD_Ipackssdw) CASE(UD_Ipackuswb) CASE(UD_Ipaddb) CASE(UD_Ipaddw)
		CASE(UD_Ipaddd) CASE(UD_Ipaddsb) CASE(UD_Ipaddsw) CASE(UD_Ipaddusb) CASE(UD_Ipaddusw) CASE(UD_Ipand)
		CASE(UD_Ipandn) CASE(UD_Ipavgb) CASE(UD_Ipavgw) CASE(UD_Ipcmpeqb) CASE(UD_Ipcmpeqw) CASE(UD_Ipcmpeqd)
		CASE(UD_Ipcmpgtb) CASE(UD_Ipcmpgtw) CASE(UD_Ipcmpgtd) CASE(UD_Ipextrb) CASE(UD_Ipextrd) CASE(UD_Ipextrq)
		CASE(UD_Ipextrw) CASE(UD_Ipinsrw) CASE(UD_Ipmaddwd) CASE(UD_Ipmaxsw) CASE(UD_Ipmaxub) CASE(UD_Ipminsw)
		CASE(UD_Ipminub) CASE(UD_Ipmovmskb) CASE(UD_Ipmulhuw) CASE(UD_Ipmulhw) CASE(UD_Ipmullw) CASE(UD_Ipop)
		CASE(UD_Ipopa) CASE(UD_Ipopad) CASE(UD_Ipopfw) CASE(UD_Ipopfd) CASE(UD_Ipopfq) CASE(UD_Ipor)
		CASE(UD_Iprefetch) CASE(UD_Iprefetchnta) CASE(UD_Iprefetcht0) CASE(UD_Iprefetcht1) CASE(UD_Iprefetcht2) CASE(UD_Ipsadbw)
		CASE(UD_Ipshufw) CASE(UD_Ipsllw) CASE(UD_Ipslld) CASE(UD_Ipsllq) CASE(UD_Ipsraw) CASE(UD_Ipsrad)
		CASE(UD_Ipsrlw) CASE(UD_Ipsrld) CASE(UD_Ipsrlq) CASE(UD_Ipsubb) CASE(UD_Ipsubw) CASE(UD_Ipsubd)
		CASE(UD_Ipsubsb) CASE(UD_Ipsubsw) CASE(UD_Ipsubusb) CASE(UD_Ipsubusw) CASE(UD_Ipunpckhbw) CASE(UD_Ipunpckhwd)
		CASE(UD_Ipunpckhdq) CASE(UD_Ipunpcklbw) CASE(UD_Ipunpcklwd) CASE(UD_Ipunpckldq) CASE(UD_Ipi2fw) CASE(UD_Ipi2fd)
		CASE(UD_Ipf2iw) CASE(UD_Ipf2id) CASE(UD_Ipfnacc) CASE(UD_Ipfpnacc) CASE(UD_Ipfcmpge) CASE(UD_Ipfmin)
		CASE(UD_Ipfrcp) CASE(UD_Ipfrsqrt) CASE(UD_Ipfsub) CASE(UD_Ipfadd) CASE(UD_Ipfcmpgt) CASE(UD_Ipfmax)
		CASE(UD_Ipfrcpit1) CASE(UD_Ipfrsqit1) CASE(UD_Ipfsubr) CASE(UD_Ipfacc) CASE(UD_Ipfcmpeq) CASE(UD_Ipfmul)
		CASE(UD_Ipfrcpit2) CASE(UD_Ipmulhrw) CASE(UD_Ipswapd) CASE(UD_Ipavgusb) CASE(UD_Ipush) CASE(UD_Ipusha)
		CASE(UD_Ipushad) CASE(UD_Ipushfw) CASE(UD_Ipushfd) CASE(UD_Ipushfq) CASE(UD_Ipxor) CASE(UD_Ircl)
		CASE(UD_Ircr) CASE(UD_Irol) CASE(UD_Iror) CASE(UD_Ircpps) CASE(UD_Ircpss) CASE(UD_Irdmsr)
		CASE(UD_Irdpmc) CASE(UD_Irdtsc) CASE(UD_Irdtscp) CASE(UD_Irepne) CASE(UD_Irep) CASE(UD_Iret)
		CASE(UD_Iretf) CASE(UD_Irsm) CASE(UD_Irsqrtps) CASE(UD_Irsqrtss) CASE(UD_Isahf) CASE(UD_Isalc)
		CASE(UD_Isar) CASE(UD_Ishl) CASE(UD_Ishr) CASE(UD_Isbb) CASE(UD_Iscasb) CASE(UD_Iscasw)
		CASE(UD_Iscasd) CASE(UD_Iscasq) CASE(UD_Iseto) CASE(UD_Isetno) CASE(UD_Isetb) CASE(UD_Isetnb)
		CASE(UD_Isetz) CASE(UD_Isetnz) CASE(UD_Isetbe) CASE(UD_Iseta) CASE(UD_Isets) CASE(UD_Isetns)
		CASE(UD_Isetp) CASE(UD_Isetnp) CASE(UD_Isetl) CASE(UD_Isetge) CASE(UD_Isetle) CASE(UD_Isetg)
		CASE(UD_Isfence) CASE(UD_Isgdt) CASE(UD_Ishld) CASE(UD_Ishrd) CASE(UD_Ishufpd) CASE(UD_Ishufps)
		CASE(UD_Isidt) CASE(UD_Isldt) CASE(UD_Ismsw) CASE(UD_Isqrtps) CASE(UD_Isqrtpd) CASE(UD_Isqrtsd)
		CASE(UD_Isqrtss) CASE(UD_Istc) CASE(UD_Istd) CASE(UD_Istgi) CASE(UD_Isti) CASE(UD_Iskinit)
		CASE(UD_Istmxcsr) CASE(UD_Istosb) CASE(UD_Istosw) CASE(UD_Istosd) CASE(UD_Istosq) CASE(UD_Istr)
		CASE(UD_Isub) CASE(UD_Isubpd) CASE(UD_Isubps) CASE(UD_Isubsd) CASE(UD_Isubss) CASE(UD_Iswapgs)
		CASE(UD_Isyscall) CASE(UD_Isysenter) CASE(UD_Isysexit) CASE(UD_Isysret) CASE(UD_Itest) CASE(UD_Iucomisd)
		CASE(UD_Iucomiss) CASE(UD_Iud2) CASE(UD_Iunpckhpd) CASE(UD_Iunpckhps) CASE(UD_Iunpcklps) CASE(UD_Iunpcklpd)
		CASE(UD_Iverr) CASE(UD_Iverw) CASE(UD_Ivmcall) CASE(UD_Ivmclear) CASE(UD_Ivmxon) CASE(UD_Ivmptrld)
		CASE(UD_Ivmptrst) CASE(UD_Ivmlaunch) CASE(UD_Ivmresume) CASE(UD_Ivmxoff) CASE(UD_Ivmread) CASE(UD_Ivmwrite)
		CASE(UD_Ivmrun) CASE(UD_Ivmmcall) CASE(UD_Ivmload) CASE(UD_Ivmsave) CASE(UD_Iwait) CASE(UD_Iwbinvd)
		CASE(UD_Iwrmsr) CASE(UD_Ixadd) CASE(UD_Ixchg) CASE(UD_Ixlatb) CASE(UD_Ixor) CASE(UD_Ixorpd)
		CASE(UD_Ixorps) CASE(UD_Ixcryptecb) CASE(UD_Ixcryptcbc) CASE(UD_Ixcryptctr) CASE(UD_Ixcryptcfb) CASE(UD_Ixcryptofb)
		CASE(UD_Ixsha1) CASE(UD_Ixsha256) CASE(UD_Ixstore) CASE(UD_Imovdqa) CASE(UD_Imovdq2q) CASE(UD_Imovdqu)
		CASE(UD_Imovq2dq) CASE(UD_Ipaddq) CASE(UD_Ipsubq) CASE(UD_Ipmuludq) CASE(UD_Ipshufhw) CASE(UD_Ipshuflw)
		CASE(UD_Ipshufd) CASE(UD_Ipslldq) CASE(UD_Ipsrldq) CASE(UD_Ipunpckhqdq) CASE(UD_Ipunpcklqdq) CASE(UD_Iaddsubpd)
		CASE(UD_Iaddsubps) CASE(UD_Ihaddpd) CASE(UD_Ihaddps) CASE(UD_Ihsubpd) CASE(UD_Ihsubps) CASE(UD_Imovddup)
		CASE(UD_Imovshdup) CASE(UD_Imovsldup) CASE(UD_Ipabsb) CASE(UD_Ipabsw) CASE(UD_Ipabsd) CASE(UD_Ipsignb)
		CASE(UD_Iphaddw) CASE(UD_Iphaddd) CASE(UD_Iphaddsw) CASE(UD_Ipmaddubsw) CASE(UD_Iphsubw) CASE(UD_Iphsubd)
		CASE(UD_Iphsubsw) CASE(UD_Ipsignd) CASE(UD_Ipsignw) CASE(UD_Ipmulhrsw) CASE(UD_Ipalignr) CASE(UD_Ipblendvb)
		CASE(UD_Ipmuldq) CASE(UD_Ipminsb) CASE(UD_Ipminsd) CASE(UD_Ipminuw) CASE(UD_Ipminud) CASE(UD_Ipmaxsb)
		CASE(UD_Ipmaxsd) CASE(UD_Ipmaxud) CASE(UD_Ipmulld) CASE(UD_Iphminposuw) CASE(UD_Iroundps) CASE(UD_Iroundpd)
		CASE(UD_Iroundss) CASE(UD_Iroundsd) CASE(UD_Iblendpd) CASE(UD_Ipblendw) CASE(UD_Iblendps) CASE(UD_Iblendvpd)
		CASE(UD_Iblendvps) CASE(UD_Idpps) CASE(UD_Idppd) CASE(UD_Impsadbw) CASE(UD_Iextractps)
		default:
			return "???";
	}
#undef CASE
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
	MODIFY_SINGLE(UD_Idec);
	MODIFY_SINGLE(UD_Iinc);
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
	NO_MODIFICATION(UD_Ijl);
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
	MODIFY_SINGLE(UD_Iidiv);
	MODIFY_SINGLE(UD_Iimul);
	NO_MODIFICATION(UD_Inop);
	MODIFY_SINGLE(UD_Inot);
	MODIFY_SINGLE(UD_Ior);
	MODIFY_SINGLE(UD_Ipop);
	NO_MODIFICATION(UD_Ipush);
	MODIFY_SINGLE(UD_Isar);
	MODIFY_SINGLE(UD_Isetl);
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
		str << __func__ << ": Unknown mnemonic " << Udis86Helper::mnemonicToString(ud->mnemonic)
		    << " (0x" << std::hex << boost::lexical_cast<std::string>(ud->mnemonic)
		    << ") :: " << ud_insn_asm(ud);
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
