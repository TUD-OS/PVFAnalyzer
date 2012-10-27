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
#include <boost/graph/strong_components.hpp>
#include <boost/graph/graphviz.hpp>
#include "instruction/cfg.h"
#include "util.h"

struct PVFConfig : public Configuration
{
	std::string input_filename;
	std::string output_filename;
	Address     final;

	PVFConfig()
		: Configuration(), input_filename("output.ilist"),
	      output_filename("output.pvf"), final(0)
	{ }
};

static PVFConfig config;


static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-v] [-d]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Set input file [output.ilist]" << std::endl;
	std::cout << "\t-o <file>          Write the resulting output to file. [output.pvf]" << std::endl;
	std::cout << "\t-t <addr>          Set PVF termination address [0x00000000]" << std::endl;
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
	std::cout << "\033[33m" << "      PVF::Regs Analyzer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "df:ho:t:v")) != -1) {

		if (config.parse_option(opt))
			continue;

		switch(opt) {

			case 'f':
				config.input_filename = optarg;
				break;

			case 'h':
				usage(argv[0]);
				return false;

			case 'o':
				config.output_filename = optarg;
				break;

			case 't':
				config.final = Address(strtoul(optarg, 0, 0));
				break;
		}
	}
	return true;
}


typedef std::vector<Instruction::RegisterAccessInfo> RegisterAccessInfoList;
typedef std::vector<Instruction*> InstructionList;
typedef std::vector<int*> RegisterHistory;

enum States {
	UNKNOWN,
	DONTCARE,
	IMPORTANT,
	READINSTANT,
	WRITEINSTANT,
	READWRITEINSTANT,
};


static void
readIList(InstructionList& ilist)
{
	try {
		std::ifstream ifs(config.input_filename);
		if (!ifs.good()) {
			throw FileNotFoundException(config.input_filename.c_str());
		}
		boost::archive::binary_iarchive ia(ifs);
		ia >> ilist;
		ifs.close();
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}


static void obtainRegisterAccessInfo(Instruction *i, RegisterAccessInfoList& read, RegisterAccessInfoList& write)
{
	try {
		i->getRegisterRWInfo(read, write);
	} catch (NotImplementedException e) {
		std::cout << "ERROR: " << e.message << std::endl;
		i->print();
		exit(1);
	}

	if (Configuration::get()->debug) {
		std::cout << "  READ  regs: [ ";
		BOOST_FOREACH(Instruction::RegisterAccessInfo info, read) {
			std::cout << std::dec << "(" << info.first << "," << info.second << ") ";
		}
		std::cout << "]" << std::endl;

		std::cout << "  WRITE regs: [ ";
		BOOST_FOREACH(Instruction::RegisterAccessInfo info, write) {
			std::cout << std::dec << "(" << info.first << "," << info.second << ") ";
		}
		std::cout << "]" << std::endl;
	}
}


static void dumpHistory(InstructionList& ilist, RegisterHistory& hist)
{
	for (unsigned t = 0; t < hist.size(); ++t) {
		std::cout << std::setfill(' ') << std::setw(40) << ilist[t]->c_str() << " | ";
		for (unsigned i = 0; i < PlatformX8632::numGPRs(); ++i) {
			std::cout << std::setw(6);
			switch(hist[t][i]) {
				case DONTCARE:         std::cout << "-"; break;
				case UNKNOWN:          std::cout << "?"; break;
				case IMPORTANT:        std::cout << "X"; break;
				case READINSTANT:      std::cout << "R"; break;
				case WRITEINSTANT:     std::cout << "W"; break;
				case READWRITEINSTANT: std::cout << "M"; break;
			}
		}
		std::cout << std::endl;
	}
}


static void
pvfAnalysis(InstructionList& ilist)
{
	std::cout << std::setw(40) << "Instruction" << " | ";
	for (unsigned i = 0; i < PlatformX8632::numGPRs(); ++i)
	{
		std::cout << std::setw(6) << PlatformX8632::RegisterToString((PlatformX8632::Register)i);
	}
	std::cout << std::endl;

	RegisterHistory hist;
	unsigned timestamp = 0;

	BOOST_FOREACH(Instruction* i, ilist) {
		if (Configuration::get()->debug) {
			i->print();
			std::cout << std::endl;
		}

		std::vector<Instruction::RegisterAccessInfo> read, write;
		obtainRegisterAccessInfo(i, read, write);

		int *state = new int[PlatformX8632::numGPRs()];

		/* 1) Initialize all GPR entries for this instruction to UNKNOWN */
		DEBUG(std::cout << "1  [new st @ " << (void*)state << std::endl;);
		for (unsigned i = 0; i < PlatformX8632::numGPRs(); ++i) {
			state[i] = UNKNOWN;
		}

		/* 2) Handle WRITE and READWRITE accesses */
		DEBUG(std::cout << "2" << std::endl;);
		BOOST_FOREACH(Instruction::RegisterAccessInfo info, write) {
			state[info.first] = WRITEINSTANT;

			/*
			 * This value was written to. All previous instants since
			 * the last READ are DONTCARE. Exception: if this register is INOUT
			 * (e.g., it is contained in the read set), we skip this part.
			 */
			bool skipIt = false;
			BOOST_FOREACH(Instruction::RegisterAccessInfo rai, read) {
				if (rai.first == info.first) {
					state[info.first] = READWRITEINSTANT;
					skipIt = true;
				}
			}

			int check = timestamp-1;
			while ((!skipIt) and (check >= 0)) {
				DEBUG(std::cout << "2." << check << std::endl;);
				int *st = hist[check];
				DEBUG(std::cout << " st @ " << (void*)st << std::endl;);
				DEBUG(std::cout << " st = " << st[0] << std::endl;);
				if ((st[info.first] == READINSTANT) or
					(st[info.first] == READWRITEINSTANT)) {
					break;
				}
				st[info.first] = DONTCARE;
				check--;
			}
		}

		/* 3) Handle READ accesses */
		DEBUG(std::cout << "3" << std::endl;);
		BOOST_FOREACH(Instruction::RegisterAccessInfo info, read) {
			if (state[info.first] == UNKNOWN) {
				state[info.first] = READINSTANT;
			}
			/*
			 * The value was read, so we assume everything since the last
			 * write access to be important.
			 */
			int check = timestamp - 1;
			while (check >= 0) {
				int* st = hist[check];
				if ((st[info.first] == WRITEINSTANT) or
					(st[info.first] == READWRITEINSTANT)) {
					break;
				}
				st[info.first] = IMPORTANT;
				check--;
			}
		}

		/* 4) Adjust anything else according to the previous state. */
		DEBUG(std::cout << "4" << std::endl;);
		if (hist.size() > 0) {
			int* prevState = hist.back();
			for (unsigned i=0; i < PlatformX8632::numGPRs(); ++i) {

				if ((state[i] == READINSTANT) or (state[i] == WRITEINSTANT) or
					(state[i] == READWRITEINSTANT)) {
					continue;
				}

				switch (prevState[i]) {
					case DONTCARE:
					case UNKNOWN:
					case IMPORTANT:
						state[i] = prevState[i];
						break;
					case READINSTANT:
					case WRITEINSTANT:
					case READWRITEINSTANT:
						state[i] = UNKNOWN;
						break;
				}
			}
		}

		/* 5) Store new state. */
		DEBUG(std::cout << "5" << std::endl;);
		hist.push_back(state);
		if (Configuration::get()->debug) {
			dumpHistory(ilist, hist);
			std::cout << "--------------------------------------------------------" << std::endl;
		}
		timestamp += 1;
	}

	assert(timestamp == hist.size());
	assert(hist.size() == ilist.size());

	dumpHistory(ilist, hist);
	BOOST_FOREACH(int* r, hist) {
		delete []r;
	}
};


int main(int argc, char **argv)
{
	Configuration::setConfig(&config);

	if (not parseInputFromOptions(argc, argv))
		exit(2);

	banner();

	InstructionList ilist;
	readIList(ilist);
	pvfAnalysis(ilist);

	/*
	 * Boost::Serialization tries to be clever w.r.t. the 
	 * pointers we store in the serialized file. If a pointer
	 * occurs multiple times, we only get one instance in the
	 * deserialized version as well. Therefore, we now need to
	 * make sure that we also only release the respective memory
	 * only once.
	 */
	std::map<Instruction*, bool> delMap;
	BOOST_FOREACH(Instruction* i, ilist) {
		if (!delMap[i]) {
			delete i;
			delMap[i] = true;
		}
	}

	return 0;
}

#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
