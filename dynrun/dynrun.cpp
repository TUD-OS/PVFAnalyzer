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

#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

/*
 * When running 32bit binaries, we need to use
 * this struct (which is in sys/user.h as well, but
 * only gets enabled if __WORDSIZE==32, which isn't
 * the case on 64bit machines.
 */
struct user_regs_struct32
{
  long int ebx;
  long int ecx;
  long int edx;
  long int esi;
  long int edi;
  long int ebp;
  long int eax;
  long int xds;
  long int xes;
  long int xfs;
  long int xgs;
  long int orig_eax;
  long int eip;
  long int xcs;
  long int eflags;
  long int esp;
  long int xss;
};


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
		CFGFromFile(cfg, config.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


static std::map<int,std::string> syscallnames;
std::string syscall2Name(int syscall)
{
	static bool init = false;
	if (!init) {
		syscallnames[0]="read";
		syscallnames[1]="write";
		syscallnames[2]="open";
		syscallnames[3]="close";
		syscallnames[4]="stat";
		syscallnames[5]="fstat";
		syscallnames[6]="lstat";
		syscallnames[7]="poll";
		syscallnames[8]="lseek";
		syscallnames[9]="mmap";
		syscallnames[10]="mprotect";
		syscallnames[11]="munmap";
		syscallnames[12]="brk";
		syscallnames[13]="rt_sigaction";
		syscallnames[14]="rt_sigprocmask";
		syscallnames[15]="rt_sigreturn";
		syscallnames[16]="ioctl";
		syscallnames[17]="pread64";
		syscallnames[18]="pwrite64";
		syscallnames[19]="readv";
		syscallnames[20]="writev";
		syscallnames[21]="access";
		syscallnames[22]="pipe";
		syscallnames[23]="select";
		syscallnames[24]="sched_yield";
		syscallnames[25]="mremap";
		syscallnames[26]="msync";
		syscallnames[27]="mincore";
		syscallnames[28]="madvise";
		syscallnames[29]="shmget";
		syscallnames[30]="shmat";
		syscallnames[31]="shmctl";
		syscallnames[32]="dup";
		syscallnames[33]="dup2";
		syscallnames[34]="pause";
		syscallnames[35]="nanosleep";
		syscallnames[36]="getitimer";
		syscallnames[37]="alarm";
		syscallnames[38]="setitimer";
		syscallnames[39]="getpid";
		syscallnames[40]="sendfile";
		syscallnames[41]="socket";
		syscallnames[42]="connect";
		syscallnames[43]="accept";
		syscallnames[44]="sendto";
		syscallnames[45]="recvfrom";
		syscallnames[46]="sendmsg";
		syscallnames[47]="recvmsg";
		syscallnames[48]="shutdown";
		syscallnames[49]="bind";
		syscallnames[50]="listen";
		syscallnames[51]="getsockname";
		syscallnames[52]="getpeername";
		syscallnames[53]="socketpair";
		syscallnames[54]="setsockopt";
		syscallnames[55]="getsockopt";
		syscallnames[56]="clone";
		syscallnames[57]="fork";
		syscallnames[58]="vfork";
		syscallnames[59]="execve";
		syscallnames[60]="exit";
		syscallnames[61]="wait4";
		syscallnames[62]="kill";
		syscallnames[63]="uname";
		syscallnames[64]="semget";
		syscallnames[65]="semop";
		syscallnames[66]="semctl";
		syscallnames[67]="shmdt";
		syscallnames[68]="msgget";
		syscallnames[69]="msgsnd";
		syscallnames[70]="msgrcv";
		syscallnames[71]="msgctl";
		syscallnames[72]="fcntl";
		syscallnames[73]="flock";
		syscallnames[74]="fsync";
		syscallnames[75]="fdatasync";
		syscallnames[76]="truncate";
		syscallnames[77]="ftruncate";
		syscallnames[78]="getdents";
		syscallnames[79]="getcwd";
		syscallnames[80]="chdir";
		syscallnames[81]="fchdir";
		syscallnames[82]="rename";
		syscallnames[83]="mkdir";
		syscallnames[84]="rmdir";
		syscallnames[85]="creat";
		syscallnames[86]="link";
		syscallnames[87]="unlink";
		syscallnames[88]="symlink";
		syscallnames[89]="readlink";
		syscallnames[90]="chmod";
		syscallnames[91]="fchmod";
		syscallnames[92]="chown";
		syscallnames[93]="fchown";
		syscallnames[94]="lchown";
		syscallnames[95]="umask";
		syscallnames[96]="gettimeofday";
		syscallnames[97]="getrlimit";
		syscallnames[98]="getrusage";
		syscallnames[99]="sysinfo";
		syscallnames[100]="times";
		syscallnames[101]="ptrace";
		syscallnames[102]="getuid";
		syscallnames[103]="syslog";
		syscallnames[104]="getgid";
		syscallnames[105]="setuid";
		syscallnames[106]="setgid";
		syscallnames[107]="geteuid";
		syscallnames[108]="getegid";
		syscallnames[109]="setpgid";
		syscallnames[110]="getppid";
		syscallnames[111]="getpgrp";
		syscallnames[112]="setsid";
		syscallnames[113]="setreuid";
		syscallnames[114]="setregid";
		syscallnames[115]="getgroups";
		syscallnames[116]="setgroups";
		syscallnames[117]="setresuid";
		syscallnames[118]="getresuid";
		syscallnames[119]="setresgid";
		syscallnames[120]="getresgid";
		syscallnames[121]="getpgid";
		syscallnames[122]="setfsuid";
		syscallnames[123]="setfsgid";
		syscallnames[124]="getsid";
		syscallnames[125]="capget";
		syscallnames[126]="capset";
		syscallnames[127]="rt_sigpending";
		syscallnames[128]="rt_sigtimedwait";
		syscallnames[129]="rt_sigqueueinfo";
		syscallnames[130]="rt_sigsuspend";
		syscallnames[131]="sigaltstack";
		syscallnames[132]="utime";
		syscallnames[133]="mknod";
		syscallnames[134]="uselib";
		syscallnames[135]="personality";
		syscallnames[136]="ustat";
		syscallnames[137]="statfs";
		syscallnames[138]="fstatfs";
		syscallnames[139]="sysfs";
		syscallnames[140]="getpriority";
		syscallnames[141]="setpriority";
		syscallnames[142]="sched_setparam";
		syscallnames[143]="sched_getparam";
		syscallnames[144]="sched_setscheduler";
		syscallnames[145]="sched_getscheduler";
		syscallnames[146]="sched_get_priority_max";
		syscallnames[147]="sched_get_priority_min";
		syscallnames[148]="sched_rr_get_interval";
		syscallnames[149]="mlock";
		syscallnames[150]="munlock";
		syscallnames[151]="mlockall";
		syscallnames[152]="munlockall";
		syscallnames[153]="vhangup";
		syscallnames[154]="modify_ldt";
		syscallnames[155]="pivot_root";
		syscallnames[156]="_sysctl";
		syscallnames[157]="prctl";
		syscallnames[158]="arch_prctl";
		syscallnames[159]="adjtimex";
		syscallnames[160]="setrlimit";
		syscallnames[161]="chroot";
		syscallnames[162]="sync";
		syscallnames[163]="acct";
		syscallnames[164]="settimeofday";
		syscallnames[165]="mount";
		syscallnames[166]="umount2";
		syscallnames[167]="swapon";
		syscallnames[168]="swapoff";
		syscallnames[169]="reboot";
		syscallnames[170]="sethostname";
		syscallnames[171]="setdomainname";
		syscallnames[172]="iopl";
		syscallnames[173]="ioperm";
		syscallnames[174]="create_module";
		syscallnames[175]="init_module";
		syscallnames[176]="delete_module";
		syscallnames[177]="get_kernel_syms";
		syscallnames[178]="query_module";
		syscallnames[179]="quotactl";
		syscallnames[180]="nfsservctl";
		syscallnames[181]="getpmsg";
		syscallnames[182]="putpmsg";
		syscallnames[183]="afs_syscall";
		syscallnames[184]="tuxcall";
		syscallnames[185]="security";
		syscallnames[186]="gettid";
		syscallnames[187]="readahead";
		syscallnames[188]="setxattr";
		syscallnames[189]="lsetxattr";
		syscallnames[190]="fsetxattr";
		syscallnames[191]="getxattr";
		syscallnames[192]="lgetxattr";
		syscallnames[193]="fgetxattr";
		syscallnames[194]="listxattr";
		syscallnames[195]="llistxattr";
		syscallnames[196]="flistxattr";
		syscallnames[197]="removexattr";
		syscallnames[198]="lremovexattr";
		syscallnames[199]="fremovexattr";
		syscallnames[200]="tkill";
		syscallnames[201]="time";
		syscallnames[202]="futex";
		syscallnames[203]="sched_setaffinity";
		syscallnames[204]="sched_getaffinity";
		syscallnames[205]="set_thread_area";
		syscallnames[206]="io_setup";
		syscallnames[207]="io_destroy";
		syscallnames[208]="io_getevents";
		syscallnames[209]="io_submit";
		syscallnames[210]="io_cancel";
		syscallnames[211]="get_thread_area";
		syscallnames[212]="lookup_dcookie";
		syscallnames[213]="epoll_create";
		syscallnames[214]="epoll_ctl_old";
		syscallnames[215]="epoll_wait_old";
		syscallnames[216]="remap_file_pages";
		syscallnames[217]="getdents64";
		syscallnames[218]="set_tid_address";
		syscallnames[219]="restart_syscall";
		syscallnames[220]="semtimedop";
		syscallnames[221]="fadvise64";
		syscallnames[222]="timer_create";
		syscallnames[223]="timer_settime";
		syscallnames[224]="timer_gettime";
		syscallnames[225]="timer_getoverrun";
		syscallnames[226]="timer_delete";
		syscallnames[227]="clock_settime";
		syscallnames[228]="clock_gettime";
		syscallnames[229]="clock_getres";
		syscallnames[230]="clock_nanosleep";
		syscallnames[231]="exit_group";
		syscallnames[232]="epoll_wait";
		syscallnames[233]="epoll_ctl";
		syscallnames[234]="tgkill";
		syscallnames[235]="utimes";
		syscallnames[236]="vserver";
		syscallnames[237]="mbind";
		syscallnames[238]="set_mempolicy";
		syscallnames[239]="get_mempolicy";
		syscallnames[240]="mq_open";
		syscallnames[241]="mq_unlink";
		syscallnames[242]="mq_timedsend";
		syscallnames[243]="mq_timedreceive";
		syscallnames[244]="mq_notify";
		syscallnames[245]="mq_getsetattr";
		syscallnames[246]="kexec_load";
		syscallnames[247]="waitid";
		syscallnames[248]="add_key";
		syscallnames[249]="request_key";
		syscallnames[250]="keyctl";
		syscallnames[251]="ioprio_set";
		syscallnames[252]="ioprio_get";
		syscallnames[253]="inotify_init";
		syscallnames[254]="inotify_add_watch";
		syscallnames[255]="inotify_rm_watch";
		syscallnames[256]="migrate_pages";
		syscallnames[257]="openat";
		syscallnames[258]="mkdirat";
		syscallnames[259]="mknodat";
		syscallnames[260]="fchownat";
		syscallnames[261]="futimesat";
		syscallnames[262]="newfstatat";
		syscallnames[263]="unlinkat";
		syscallnames[264]="renameat";
		syscallnames[265]="linkat";
		syscallnames[266]="symlinkat";
		syscallnames[267]="readlinkat";
		syscallnames[268]="fchmodat";
		syscallnames[269]="faccessat";
		syscallnames[270]="pselect6";
		syscallnames[271]="ppoll";
		syscallnames[272]="unshare";
		syscallnames[273]="set_robust_list";
		syscallnames[274]="get_robust_list";
		syscallnames[275]="splice";
		syscallnames[276]="tee";
		syscallnames[277]="sync_file_range";
		syscallnames[278]="vmsplice";
		syscallnames[279]="move_pages";
		syscallnames[280]="utimensat";
		syscallnames[281]="epoll_pwait";
		syscallnames[282]="signalfd";
		syscallnames[283]="timerfd_create";
		syscallnames[284]="eventfd";
		syscallnames[285]="fallocate";
		syscallnames[286]="timerfd_settime";
		syscallnames[287]="timerfd_gettime";
		syscallnames[288]="accept4";
		syscallnames[289]="signalfd4";
		syscallnames[290]="eventfd2";
		syscallnames[291]="epoll_create1";
		syscallnames[292]="dup3";
		syscallnames[293]="pipe2";
		syscallnames[294]="inotify_init1";
		syscallnames[295]="preadv";
		syscallnames[296]="pwritev";
		syscallnames[297]="rt_tgsigqueueinfo";
		syscallnames[298]="perf_event_open";
		syscallnames[299]="recvmmsg";
		syscallnames[300]="fanotify_init";
		syscallnames[301]="fanotify_mark";
		syscallnames[302]="prlimit64";
		syscallnames[303]="name_to_handle_at";
		syscallnames[304]="open_by_handle_at";
		syscallnames[305]="clock_adjtime";
		syscallnames[306]="syncfs";
		syscallnames[307]="sendmmsg";
		syscallnames[308]="setns";
		syscallnames[309]="getcpu";
		syscallnames[310]="process_vm_readv";
		syscallnames[311]="process_vm_writev";
	}
	return syscallnames[syscall];
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
	pid_t    _child;
	unsigned _emulationMode;

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
		//DEBUG(std::cout << "ptraced: " << r << std::endl;);

		/*
		 * A return value != 0 is ok for PTRACE_PEEK*
		 */
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
		int res = waitpid(chld, status, __WALL);
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

#if __WORDSIZE == 64
	void dumpReg(unsigned long reg)
	{
		std::cout << std::setw(16) << std::setfill('0') << std::hex << reg;
	}
#else
	void dumpRegs(unsigned reg)
	{
		std::cout << std::setw(8) << std::setfill('0') << std::hex << reg;
	}
#endif

	void dumpRegs(struct user_regs_struct* regs)
	{
		std::cout << "----------------------------------------------------------------------" << std::endl;
		std::cout << "REGS" << std::endl;
#if __WORDSIZE == 64
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
		std::cout << "EAX "; dumpRegs(regs->eax);
		std::cout << "EBX "; dumpRegs(regs->ebx);
		std::cout << "ECX "; dumpRegs(regs->ecx);
		std::cout << "EDX "; dumpRegs(regs->edx); std::cout << std::endl;
		std::cout << "ESI "; dumpRegs(regs->esi);
		std::cout << "EDI "; dumpRegs(regs->edi);
		std::cout << "EBP "; dumpRegs(regs->ebp);
		std::cout << "ESP "; dumpRegs(regs->esp); std::cout << std::endl;
		std::cout << "FLG "; dumpRegs(regs->eflags);
		std::cout << "EIP "; dumpRegs(regs->eip);
		std::cout << "ORA "; dumpRegs(regs->orig_eax); std::cout << std::endl;
#endif
		std::cout << "----------------------------------------------------------------------" << std::endl;
	}

	int ptraceSyscall32()
	{
		struct user_regs_struct data;
		int status, signal;
		int syscall;

		ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
		dumpRegs(&data);
		syscall = data.eax;
		std::cout << "   System call: " << std::dec << syscall
		          << " \033[33m(" << syscall2Name(syscall) << ")\033[0m"
		          << std::endl;

		// do system call
		ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
		if ((syscall != SYS_exit) and (syscall != SYS_exit_group)) {
			wait_checked(_child, &status, &signal);
			ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
			std::cout << "   System call return: 0x" << std::hex
			          << ((struct user_regs_struct32*)&data)->eax
			          << " " << ((struct user_regs_struct32*)&data)->orig_eax
			          << std::endl;
			return 0;
		}
		return 1;
	}

#if __WORDSIZE == 64
	int ptraceSyscall64()
	{
		struct user_regs_struct data;
		int status, signal;
		int syscall;

		ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
		dumpRegs(&data);
		syscall = data.orig_rax;
		std::cout << "   System call: " << std::dec << (data.orig_rax)
		          << " \033[33m(" << syscall2Name(data.orig_rax) << ")\033[0m"
		          << std::endl;

		// do system call
		ptrace_checked(PTRACE_SYSCALL, _child, 0, 0);
		if ((syscall != SYS_exit) and (syscall != SYS_exit_group)) {
			wait_checked(_child, &status, &signal);
			ptrace_checked(PTRACE_GETREGS, _child, 0, &data);
			std::cout << "   System call return: 0x" << std::hex
			          << (data.rax) << " " << data.orig_rax
			          << std::endl;

			/* The forked child does one execve() system call. Afterwards,
			 * we know that we are executing a 32bit binary and need to
			 * switch modes.
			 *
			 * XXX: This would be the point to look at the actual binary
			 *      and determine its mode (e.g., by looking at its ELF
			 *      class info), once we support analyzing 64bit binaries.
			 */
			if (syscall == SYS_execve) {
				_emulationMode = 32;
			}

			return 0;
		}
		return 1;
	}
#endif


	/**
	 * @brief Perform system call handling
	 *
	 * @return int
	 **/
	int handleSyscall()
	{
		/*
		 * On 64bit machines the analyzer runs in 64bit mode and hence
		 * we need to use 64bit ptrace user structs to access system
		 * call info. Later, we execve() our 32bit analysis binary
		 * and hence need to use 32bit syscall struct.
		 */
#if __WORDSIZE==64
		if (_emulationMode == 32) {
			return ptraceSyscall32();
		} else {
			return ptraceSyscall64();
		}
#else
		return ptraceSyscall32();
#endif
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
		: _child(child), _emulationMode(__WORDSIZE)
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
runInstrumented(std::list<Address> iPoints, int argc, char** argv)
{
	pid_t child = fork();
	if (child == 0) {
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);
		for (int i = 0; i < argc; ++i)
			std::cout << argv[i] << " ";
		std::cout << std::endl;
		std::cout << std::endl;
		execve(argv[0], argv, environ);
		perror("execve");
		throw ThisShouldNeverHappenException("execve failed");
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

	int newArg;
	if ((newArg = parseInputFromOptions(argc, argv)) < 0)
		exit(2);

	std::cout << "Newarg: " << newArg << std::endl;
	std::cout << "    " << argv[newArg] << std::endl;

	banner();

	ControlFlowGraph cfg;
	readCFG(cfg);

	std::list<Address> unresolved;
	int numUnres = getUnresolvedAddresses(cfg, unresolved);
	std::cout << std::dec << numUnres << " unresolved jumps." << std::endl;

	runInstrumented(unresolved, argc - newArg + 1, &argv[newArg]);

	return 0;
}


#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
