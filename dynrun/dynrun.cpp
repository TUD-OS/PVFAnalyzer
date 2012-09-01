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

#include <iostream>	          // std::cout
#include <getopt.h>	          // getopt()
#include <boost/foreach.hpp>  // FOREACH
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/graph/graphviz.hpp>
#include "instruction/cfg.h"
#include "util.h"

#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

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
	std::cout << prog << " [-h] [-f <file>] [-b <file>] [-v] [-d]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set input file [output.cfg]" << std::endl;
	std::cout << "\t-b <file>          Set input binary [input.bin]" << std::endl;
	//std::cout << "\t-o <file>          Write the resulting output to file. [output.pvf]" << std::endl;
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


static bool
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
				return false;
		}
	}
	return true;
}


static void
readCFG(ControlFlowGraph& cfg)
{
	try {
		CFGFromFile(cfg, config.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


int getUnresolvedAddresses(ControlFlowGraph const &cfg, std::list<Address>& unresolved)
{
	int count = 0;
	CFGVertexIterator v, v_end;

	for (boost::tie(v, v_end) = boost::vertices(cfg); v != v_end; ++v)
	{
		//std::cout << *v;
		CFGNodeInfo const& n = cfg[*v];
		switch(n.bb->branchType) {
			case Instruction::BT_CALL_RESOLVE:
				//std::cout << " UNRES " << std::hex << n.bb->lastInstruction().v;
				unresolved.push_back(n.bb->lastInstruction());
				count++;
				break;
			default:
				break;
		}
		//std::cout << std::endl;
	}

	return count;
}


class PTracer
{
	/**
	 * @brief Child we are tracing
	 **/
	pid_t _child;

	/**
	 * @brief Run ptrace() with arguments and print error if necessary
	 *
	 * @param req ...
	 * @param chld ...
	 * @param addr ...
	 * @param data ...
	 * @return int
	 **/
	int ptrace_checked(enum __ptrace_request req, pid_t chld, void *addr, void *data)
	{
		int r = ptrace(req, chld, addr, data);
		if (r) {
			switch(req) {
				case PTRACE_PEEKDATA:
				case PTRACE_PEEKTEXT:
				case PTRACE_PEEKUSER:
					break;
				default:
					std::cout << "   ptrace() error: " << r << std::endl;
					perror("ptrace");
			}
		}
		return r;
	}

	enum class WaitRet {
		UNKNOWN,
		TERMINATE,
		SIGNAL,
	};

	/**
	 * @brief Wait for child
	 *
	 * @param chld ...
	 * @param status ...
	 * @param signal ...
	 * @return :WaitRet
	 **/
	WaitRet wait_checked(pid_t chld, int *status, int *signal)
	{
		int res = waitpid(chld, status, 0);
		//DEBUG(std::cout << "WAIT(): res " << res << ", status " << *status << std::endl;);

		if (WIFEXITED(status)) {
			terminate(WEXITSTATUS(*status));
			// we're done
			return WaitRet::TERMINATE;
		}

		if (res != _child) {
			if (WIFSIGNALED(*status)) {
				terminate(WTERMSIG(*status));
				// we're done
				return WaitRet::TERMINATE;
			}

			DEBUG(std::cout << "   \033[31;1mwait() returned non-child status.\033[0m" << std::endl;);
			exit(1);
		} else {
			if (WIFSTOPPED(*status)) {
				*signal = WSTOPSIG(*status);
				DEBUG(std::cout << "   stopped with signal: " << *signal << std::endl;);
				return WaitRet::SIGNAL;
			}
		}
		return WaitRet::UNKNOWN;
	}


	/**
	 * @brief Perform system call handling
	 *
	 * @return int
	 **/
	int handleSyscall()
	{
		struct user_regs_struct data;
		int status, signal;
		int syscall;

		ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
		syscall = data.orig_rax;
		std::cout << "   System call: 0x" << std::hex << data.orig_rax << std::endl;

		// do system call
		ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
		if ((syscall != SYS_exit) and (syscall != SYS_exit_group)) {
			wait_checked(_child, &status, &signal);
			ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
			std::cout << "   System call return: 0x" << std::hex << data.rax << std::endl;
			return 0;
		}
		return 1;
	}

	/**
	 * @brief Print termination info
	 *
	 * @param status ...
	 * @return void
	 **/
	void terminate(int status)
	{
		std::cout << "Program terminated with status " << status << std::endl;
	}

public:
	PTracer(pid_t child)
		: _child(child)
	{ }

	/**
	 * @brief Perform initial handshake with child
	 *
	 * e.g., wait for the child to raise a SIGSTOP after
	 * calling ptrace(TRACEME...)
	 *
	 * @return bool
	 **/
	bool handshake()
	{
		int signal, status;
		WaitRet r = wait_checked(_child, &status, &signal);
		return r == WaitRet::SIGNAL and signal == SIGSTOP;
	}

	/**
	 * @brief Continue child execution
	 *
	 * @return void
	 **/
	void continueChild()
	{
		ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
	}

	/**
	 * @brief Run child and monitor ptrace signals
	 *
	 * @return void
	 **/
	void run()
	{
		int signal, status = 0;
		while (1) {
			WaitRet r = wait_checked(_child, &status, &signal);
			if (r == WaitRet::TERMINATE) {
				return;
			}
			if (r == WaitRet::SIGNAL) {
				if (signal == SIGTRAP) {
					int ret = handleSyscall();
					if (ret) {
						return;
					}
				}
			}
			continueChild();
		}
	}
};


static void
runInstrumented(std::list<Address> iPoints)
{
	pid_t child = fork();
	if (child == 0) {
		int res = ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);
		printf("abcdefghijklmnopqrstuvwxyz\n");
		exit(0);
	} else if (child > 0) {
		PTracer pt(child);

		if (!pt.handshake()) return;
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

	if (not parseInputFromOptions(argc, argv))
		exit(2);

	banner();

	ControlFlowGraph cfg;
	readCFG(cfg);

	std::list<Address> unresolved;
	int numUnres = getUnresolvedAddresses(cfg, unresolved);
	std::cout << std::dec << numUnres << " unresolved jumps." << std::endl;

	runInstrumented(unresolved);

	return 0;
}


#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
