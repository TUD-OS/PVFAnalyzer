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

#include <getopt.h>
#include <iostream>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>

#include "util.h"
#include "instruction/cfg.h"
#include "instruction/instruction_udis86.h"


struct PrinterConfiguration : public Configuration
{
	std::string input_filename;
	std::string output_filename;

	PrinterConfiguration()
		: Configuration(), input_filename("input.cfg"),
	      output_filename("output.dot")
	{ }
};

static PrinterConfiguration conf;

static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-f <file>] [-o <file>] [-v]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-d                 Debug output [off]" << std::endl;
	std::cout << "\t-f <file>          Read CFG from file. [input.cfg]" << std::endl;
	std::cout << "\t-o <file>          Write graphviz output to file. [output.dot]" << std::endl;
	std::cout << "\t                   (use -o - to print to stdout)" << std::endl;
	std::cout << "\t-v                 Verbose output [off]" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
}


static void
banner()
{
	Version version = conf.globalProgramVersion;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Printer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "f:ho:v")) != -1) {
		if (conf.parse_option(opt))
			continue;

		switch(opt) {
			case 'f':
				conf.input_filename = optarg;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'o':
				conf.output_filename = optarg;
				break;
		}
	}
	return true;
}


void readCFG(ControlFlowGraph& cfg)
{
	try {
		CFGFromFile(cfg, conf.input_filename);
	} catch (FileNotFoundException fne) {
		std::cout << "\033[31m" << fne.message << " not found.\033[0m" << std::endl;
		return;
	} catch (boost::archive::archive_exception ae) {
		std::cout << "\033[31marchive exception:\033[0m " << ae.what() << std::endl;
		return;
	}
}

void writeCFG(ControlFlowGraph& cfg)
{
	std::streambuf *buf;  // output buffer pointer
	std::ofstream ofs;    // file stream if outfile != stdout

	if (conf.output_filename == "-") { // special output file '-'
		buf = std::cout.rdbuf();
	} else {
		ofs.open(conf.output_filename);
		if (!ofs.is_open()) {
			std::cerr << "Could not open '" << conf.output_filename
			          << "' for output. Redirecting to stdout."
			          << std::endl;
			buf = std::cout.rdbuf();
		} else {
			buf = ofs.rdbuf();
		}
	}

	std::ostream out(buf); // the real output stream

	GraphvizInstructionWriter gnw(cfg);
	boost::write_graphviz(out, cfg, gnw);

	freeCFGNodes(cfg);
}


void work()
{
	ControlFlowGraph cfg;
	readCFG(cfg);
	writeCFG(cfg);
}


int main(int argc, char **argv)
{
	if (not parseInputFromOptions(argc, argv))
		return 1;

	Configuration::setConfig(&conf);

	banner();
	work();

	return 0;
}
#include "instruction/instruction_udis86.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
BOOST_CLASS_EXPORT_GUID(Udis86Instruction, "Udis86Instruction");
#pragma GCC diagnostic pop
