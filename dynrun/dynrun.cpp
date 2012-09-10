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

#include <iomanip>
#include <iostream>	          // std::cout
#include <getopt.h>	          // getopt()
#include <boost/foreach.hpp>  // FOREACH
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/graph/graphviz.hpp>
#include "instruction/cfg.h"
#include "util.h"
#include "dynrun.h"

#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>


struct DynRunConfig : public Configuration
{
	std::string input_filename;
	std::string binary;

	DynRunConfig()
		: Configuration(), input_filename("output.cfg"), binary("input.bin")
	{ }
};

static DynRunConfig config;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <CFG file>] [-v] [-d] -- <binary> <arguments>"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set input file [output.cfg]" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
}


static void
banner()
{
	Version version = Configuration::get()->globalProgramVersion;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        DynRun version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static int
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "b:df:hv")) != -1) {

		if (config.parse_option(opt))
			continue;

		switch(opt) {

			case 'b':
				config.binary = optarg;
				break;

			case 'f':
				config.input_filename = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return -1;
		}
		printf("%d\n", optind);
	}
	return optind;
}


static void
readCFG(ControlFlowGraph& cfg)
{
	try {
		cfg.fromFile(config.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


/**
 * @brief Wrappers around system calls
 **/
struct Syscalls
{
	/**
	 * @brief Run ptrace() with arguments and print error if necessary
	 *
	 * @param req ptrace request
	 * @param chld child PID
	 * @param addr ptrace address parameter
	 * @param data ptrace data parameter
	 * @return ptrace return value
	 **/
	static long ptrace_checked(enum __ptrace_request req, pid_t chld, unsigned long addr, void *data)
	{
		long r = ptrace(req, chld, addr, data);
		//DEBUG(std::cout << "ptraced: " << r << std::endl;);

		if (r and errno) {
			perror("ptrace");
		}

		return r;
	}

	/**
	 * @brief Wait for child
	 *
	 * @param chld child PID
	 * @param status status pointer
	 * @return waitpid() return value
	 **/
	static pid_t wait_checked(pid_t chld, int *status)
	{
		pid_t res = waitpid(chld, status, __WALL);
		//DEBUG(std::cout << "WAIT(): res " << res << ", status " << *status << std::endl;);
		if (res == -1) {
			perror("waitpid");
		}
		return res;
	}

	static void dumpMemory(pid_t process, Address a, size_t size)
	{
		for (unsigned long w = a.v; w < a.v + size; w += sizeof(w)) {
			unsigned long data = Syscalls::ptrace_checked(PTRACE_PEEKDATA, process, w, 0);
			std::cout << std::hex << "[0x" << w << "] ";
			for (unsigned i = 0; i < sizeof(w); i+=2) {
				std::cout << std::hex << std::setw(2) << std::setfill('0')
				          << ((data >> (i*4)) & 0xFF) << " ";
			}
			std::cout << std::endl;
		}

	}
};


/**
 * @brief Reason for which a breakpoint was set
 **/
enum class BreakpointReason
{
	BP_JUMP, // unresolved jump
};

/**
 * @brief Data stored for each breakpoint
 **/
struct BreakpointData
{
	/*
	 * Store the original BP address, because after stepping through
	 * a BP we might not know where we came from anymore.
	 */
	Address          target;

	/*
	 * Depending on the original reason to place this
	 * BP, we later decide if we want to set it again
	 */
	BreakpointReason reason;

	/*
	 * Store original data for single-stepping once we
	 * hit the BP.
	 */
	unsigned long	origData;

	/*
	 * Number of times this BP was hit.
	 */
	unsigned         hitCount;


	/*
	 * As a shortcut for extending the CFG, we also store the
	 * CFG Vertex this BP belongs to. Otherwise we'd have to do
	 * a search() for every CFGBuilder::extend().
	 */
	CFGVertexDescriptor vertex;


	BreakpointData(Address t, BreakpointReason r, CFGVertexDescriptor v = 0)
		: target(t), reason(r), origData(0), hitCount(0), vertex(v)
	{ }

	void dump()
	{
		std::cout << "BP @ 0x" << std::hex << target.v
		          << " reason: " << (int)reason
		          << " orig word: " << origData
		          << " hit count: " << std::dec << hitCount
		          << std::endl;
	}


	/**
	 * @brief Patch target's address space so that it triggers BP at target address
	 *
	 * @param process PID
	 * @return void
	 **/
	void arm(pid_t process)
	{
		DEBUG(std::cout << "arming BP @ " << std::hex << target.v << std::endl;);
		origData = Syscalls::ptrace_checked(PTRACE_PEEKDATA, process, target.v, 0);
		unsigned long newdata = (origData & ~0xFF) | 0xCC; // INT3;
		Syscalls::ptrace_checked(PTRACE_POKEDATA, process, target.v, (void*)newdata);

	}


	/**
	 * @brief Disable a previously enabled BP
	 *
	 * @param process PID
	 * @return void
	 **/
	void disarm(pid_t process)
	{
		Syscalls::ptrace_checked(PTRACE_POKEDATA, process, target.v, (void*)origData);
		Syscalls::ptrace_checked(PTRACE_POKEUSER, process,
#if __WORDSIZE == 32
		                         4 * EIP,
#else
		                         8 * RIP,
#endif
		                         (void*)target.v);
	}
};


/**
 * @brief Walk the CFG and generate a list of Breakpoints for unresolved jumps
 *
 * @param cfg CFG
 * @param unresolved output: list of breakpoints
 * @return number of breakpoints
 **/
int getUnresolvedAddresses(ControlFlowGraph const &cfg, std::list<BreakpointData*>& unresolved)
{
	CFGVertexIterator v, v_end;

	for (boost::tie(v, v_end) = boost::vertices(cfg.cfg); v != v_end; ++v)
	{
		//std::cout << *v;
		CFGNodeInfo const& n = cfg.node(*v);
		switch(n.bb->branchType) {
			case Instruction::BT_CALL_RESOLVE:
				//std::cout << " UNRES " << std::hex << n.bb->lastInstruction().v;
				unresolved.push_back(new BreakpointData(n.bb->lastInstruction(), BreakpointReason::BP_JUMP, *v));
				break;
			default:
				break;
		}
		//std::cout << std::endl;
	}

	return unresolved.size();
}


/**
 * @brief PTrace-based debugger handling our breakpoints
 **/
class PTracer
{
	/**
	 * @brief Child we are tracing
	 **/
	pid_t    _child;         // the child process
	unsigned _emulationMode; // executing in 32 or 64 bit mode?

	bool     _inSyscall;     // are we currently in a system call?
	unsigned _curSyscall;    // syscall number

	bool     _inSinglestep;  // are we currently single-stepping due to a BP?
	BreakpointData *_curBreakpoint; // current BP descriptor
	std::map<Address, BreakpointData*> _breakpoints; // list of all breakpoints

	CFGBuilder * _cfgBuilder; // CFG Builder to be called for unresolved JMP nodes

	enum {
		execve32 = 11,
		execve64 = 59,
	};


#if __WORDSIZE == 64
	void dumpReg(unsigned long reg)
	{
		std::cout << std::setw(16) << std::setfill('0') << std::hex << reg;
	}
#else
	void dumpReg(unsigned reg)
	{
		std::cout << std::setw(8) << std::setfill('0') << std::hex << reg;
	}
#endif

	void dumpRegs(void* arg)
	{
		std::cout << "----------------------------------------------------------------------" << std::endl;
		std::cout << "REGS" << std::endl;
#if __WORDSIZE == 64
		struct user_regs_struct* regs = reinterpret_cast<struct user_regs_struct*>(arg);
		std::cout << "R15 "; dumpReg(regs->r15);
		std::cout << " R14 "; dumpReg(regs->r14);
		std::cout << " R13 "; dumpReg(regs->r13);
		std::cout << " R12 "; dumpReg(regs->r12); std::cout << std::endl;
		std::cout << "R11 "; dumpReg(regs->r11);
		std::cout << " R10 "; dumpReg(regs->r10);
		std::cout << " R09 "; dumpReg(regs->r9);
		std::cout << " R08 "; dumpReg(regs->r8); std::cout << std::endl;
		std::cout << "RAX "; dumpReg(regs->rax);
		std::cout << " RBX "; dumpReg(regs->rbx);
		std::cout << " RCX "; dumpReg(regs->rcx);
		std::cout << " RDX "; dumpReg(regs->rdx); std::cout << std::endl;
		std::cout << "RSP "; dumpReg(regs->rsp);
		std::cout << " RBP "; dumpReg(regs->rbp);
		std::cout << " RIP "; dumpReg(regs->rip);
		std::cout << " FLG "; dumpReg(regs->eflags); std::cout << std::endl;
		std::cout << "ORA "; dumpReg(regs->orig_rax); std::cout << std::endl;
#else
		struct user_regs_struct* regs = reinterpret_cast<struct user_regs_struct*>(arg);
		std::cout << "EAX "; dumpReg(regs->eax);
		std::cout << " EBX "; dumpReg(regs->ebx);
		std::cout << " ECX "; dumpReg(regs->ecx);
		std::cout << " EDX "; dumpReg(regs->edx); std::cout << std::endl;
		std::cout << "ESI "; dumpReg(regs->esi);
		std::cout << " EDI "; dumpReg(regs->edi);
		std::cout << " EBP "; dumpReg(regs->ebp);
		std::cout << " ESP "; dumpReg(regs->esp); std::cout << std::endl;
		std::cout << "FLG "; dumpReg(regs->eflags);
		std::cout << " EIP "; dumpReg(regs->eip);
		std::cout << " ORA "; dumpReg(regs->orig_eax); std::cout << std::endl;
#endif
		std::cout << "----------------------------------------------------------------------" << std::endl;
	}


	/**
	 * @brief Perform system call handling
	 *
	 * @return int
	 **/
	int handleSyscall()
	{
		struct user_regs_struct data;

		Syscalls::ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
		if (Configuration::get()->debug) {
			//dumpRegs(&data);
		}

		if (!_inSyscall) {
#if __WORDSIZE == 64
			_curSyscall = data.orig_rax;
#else
			_curSyscall = data.orig_eax;
#endif
			std::cout << "   System call: " << std::dec << _curSyscall
					  << " \033[33m(" << syscall2Name(_curSyscall, _emulationMode)
					  << ")\033[0m" << std::endl;
			_inSyscall = true;
		} else {
#if __WORDSIZE == 64
			int sysret = data.rax;
#else
			int sysret = data.eax;
#endif
			std::cout << "   System call return: 0x" << std::hex
			          << sysret << std::endl;
			_inSyscall = false;

#if __WORDSIZE == 64
			if (_curSyscall == execve64) { // == SYS_execve on 64bit
				_emulationMode = 32;
				_curSyscall    = execve32; // == SYS_execve on 32bit
				_inSyscall     = true;
			}
#else
			if (_curSyscall == execve32) { // there'll be a 2nd syscall return from execve
				_inSyscall = true;
			}
#endif
		}

		return 0;
	}


	/**
	 * @brief Breakpoint handling
	 *
	 * @return int
	 **/
	int handleBreakpoint()
	{
		struct user_regs_struct regs;
		Syscalls::ptrace_checked(PTRACE_GETREGS, _child, 0, &regs);

		unsigned long ip;
#if __WORDSIZE == 32
		ip = regs.eip;
#else
		ip = regs.rip;
#endif

		if (_inSinglestep) {
			DEBUG(std::cout << "back from Singlestep. "
			                << "Orig BP: " << std::hex << _curBreakpoint->target.v
			                << " cur IP: " << ip
			                << std::endl;);

			_cfgBuilder->extend(_curBreakpoint->vertex, Address(ip));

			_curBreakpoint->arm(_child);
			_curBreakpoint = 0;
			_inSinglestep  = false;

			continueChild();
		} else {
			ip -= 1; // if this is an INT3 breakpoint, the orig BP was set at IP-1

			BreakpointData* bp = _breakpoints[Address(ip)];
			if (!bp)
				return 0;

			if (Configuration::get()->debug) {
				//bp->dump();
			}

			bp->hitCount++;
			bp->disarm(_child);
			_curBreakpoint = bp;
			_inSinglestep  = true;

			Syscalls::ptrace_checked(PTRACE_SINGLESTEP, _child, 0, 0);
		}
		
		return 1;
	}


	/**
	 * @brief Dump signal info
	 *
	 * @return void
	 **/
	void handleSignal()
	{
		siginfo_t sig;
		struct user_regs_struct regs;

		Syscalls::ptrace_checked(PTRACE_GETSIGINFO, _child, 0, &sig);
		Syscalls::ptrace_checked(PTRACE_GETREGS, _child, 0, &regs);

		std::cout << "Signal info: " << std::endl;
		std::cout << "Signal " << std::dec << sig.si_signo;
		switch(sig.si_signo) {
			case SIGSEGV:
				std::cout << " (SEGV) @ " << std::hex << sig.si_addr
				          << ", IP @ "
#if __WORDSIZE == 32
				          << regs.eip
#else
				          << regs.rip
#endif
				          << std::endl;
				dumpRegs(&regs);
				throw ThisShouldNeverHappenException("SEGV");
				break;
			default:
				std::cout << "XXX" << std::endl;
		}
	}

public:
	PTracer(pid_t child, CFGBuilder *builder)
		: _child(child), _emulationMode(__WORDSIZE),
		  _inSyscall(false), _curSyscall(~0U),
		  _inSinglestep(false), _curBreakpoint(0),
		  _breakpoints(), _cfgBuilder(builder)
	{ }

	PTracer(PTracer const&)            = delete;
	PTracer& operator=(PTracer const&) = delete;

	/**
	 * @brief Perform initial handshake with child
	 *
	 * Runs the child process until it is ready to set breakpoints
	 * (e.g., has executed the initial execve() system call).
	 *
	 * @return bool
	 **/
	bool handshake()
	{
		int status;
		int execveRetSeen = 0;
		int r = Syscalls::wait_checked(_child, &status);
		assert(r==_child and WIFSTOPPED(status) and WSTOPSIG(status) == SIGSTOP);

		for (;;) {
			continueChild();
			r = Syscalls::wait_checked(_child, &status);
			if (r == _child and WIFSTOPPED(status) and WSTOPSIG(status) == SIGTRAP) {
				handleSyscall();
#if __WORDSIZE==64
				// run in syscall emulation until our child switched to
				// 32bit execution and is in execve()
				if (_emulationMode == 32 and _curSyscall == execve32) {
					return true;
				}
#else
				if (_curSyscall == execve32) {
					execveRetSeen++;
					if (execveRetSeen > 2) {
						_inSyscall = false;
						return true;
					}
				}
#endif
			}
			else {
				std::cout << "ERR1" << std::endl;
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Continue child execution
	 *
	 * @return void
	 **/
	void continueChild()
	{
		Syscalls::ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
	}

	/**
	 * @brief Run child and monitor ptrace signals
	 *
	 * @return void
	 **/
	void run()
	{
		int status = 0;
		while (1) {
			int r = Syscalls::wait_checked(_child, &status);

			if (r == -1) {
				return;
			}

			if (WIFSTOPPED(status)) {
				DEBUG(std::cout << "Stopped with signal " << std::dec << WSTOPSIG(status) << std::endl;);
				if (WSTOPSIG(status) == SIGTRAP) {
					if (handleBreakpoint())
						continue;
					if (handleSyscall())
						return;
				} else if (WSTOPSIG(status) == SIGSEGV) {
					handleSignal();
				} else {
					return;	
				}
			}

			if (WIFEXITED(status)) {
				DEBUG(std::cout << "Terminated with status " << WEXITSTATUS(status) << std::endl;);
				return;
			}

			continueChild();
		}
	}

	/**
	 * @brief Add a BP to our list and enable it in the child's AS.
	 *
	 * @param bp Breakpoint descriptor
	 * @return void
	 **/
	
	void addBreakpoint(BreakpointData* bp)
	{
		_breakpoints[bp->target] = bp;
		bp->arm(_child);
	}
};


static void
runInstrumented(std::list<BreakpointData*> iPoints, int argc, char** argv,
                CFGBuilder* builder)
{
	pid_t child = fork();
	if (child == 0) {
		/*
		 * Child's part of the ptrace handshake
		 */
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);

		for (int i = 0; i < argc; ++i)
			std::cout << argv[i] << " ";
		std::cout << std::endl;

		/*
		 * Execute the binary including parameters and
		 * inheriting parent's environment
		 */
		execve(argv[0], argv, environ);
		perror("execve");
		throw ThisShouldNeverHappenException("execve failed");
	} else if (child > 0) {

		PTracer pt(child, builder);

		if (!pt.handshake()) {
			return;
		}

		BOOST_FOREACH(BreakpointData* bp, iPoints) {
			pt.addBreakpoint(bp);
		}
		//exit(1);

		pt.continueChild();

		pt.run();

	} else {
		perror("fork");
		exit(1);
	}
}


int main(int argc, char **argv)
{
	Configuration::setConfig(&config);

	int newArg;
	if ((newArg = parseInputFromOptions(argc, argv)) < 0)
		exit(2);

	DEBUG(std::cout << "Newarg: " << newArg << std::endl;);
	DEBUG(std::cout << "        " << argv[newArg] << std::endl;)

	banner();

	ControlFlowGraph cfg;
	std::vector<InputReader*> input;
	CFGBuilder *builder = 0;

	readCFG(cfg);

	std::list<BreakpointData*> unresolved;
	int numUnres = getUnresolvedAddresses(cfg, unresolved);
	std::cout << std::dec << numUnres << " unresolved jumps." << std::endl;

	FileInputReader *fr = new FileInputReader();
	input.push_back(fr);
	fr->addData(argv[newArg]);
	builder = CFGBuilder::get(input, cfg);

	runInstrumented(unresolved, argc - newArg + 1, &argv[newArg], builder);
	std::cout << "Program run terminated." << std::endl;

	std::cout << "Built CFG. " << std::dec << boost::num_vertices(cfg.cfg) << " vertices, "
			<< boost::num_edges(cfg.cfg) << " edges." << std::endl;
	cfg.toFile(config.input_filename);

	BOOST_FOREACH(BreakpointData* bp, unresolved) {
		delete bp;
	}

	return 0;
}


#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
