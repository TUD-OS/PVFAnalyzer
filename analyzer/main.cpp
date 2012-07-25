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

#include <iostream>		// std::cout
#include <getopt.h>		// getopt()
#include "input.h"      // RawData, InputReader
#include "disassembler.h"

/**
 * @brief Command line long options
 **/
struct option my_opts[] = {
	{"file", required_argument, 0, 'f'},
	{"help", no_argument,       0, 'h'},
	{"hex",  no_argument,       0, 'x'},
	{0,0,0,0} // this line be last
};


static void
usage(char const *prog)
{
	std::cout << "\033[32mUsage:\033[0m" << std::endl << std::endl;
	std::cout << prog << " [-h] [-x <bytestream>] [-f <file>]"
	          << std::endl << std::endl << "\033[32mOptions\033[0m" << std::endl;
	std::cout << "\t-f <file>          Parse binary file (ELF or raw binary)" << std::endl;
	std::cout << "\t-h                 Display help" << std::endl;
	std::cout << "\t-x <bytes>         Interpret the following two-digit hexadecimal numbers" << std::endl;
	std::cout << "\t                   as input to work on." << std::endl;
}


static void
banner()
{
	struct {
		unsigned minor;
		unsigned major;
	} version =  {0,0};
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
	std::cout << "\033[33m" << "        CFG Analyzer version " << version.major
	          << "." << version.minor << "\033[0m" << std::endl;
	std::cout << "\033[34m" << "********************************************"
	          << "\033[0m" << std::endl;
}


static bool
parseInputFromOptions(int argc, char **argv, DataSection& target)
{
	int opt;
	while ((opt = getopt(argc, argv, "f:hx")) != -1) {
		switch(opt) {
			case 'f': { // file input
					std::cout << "input file: " << argv[optind-1] << std::endl;
					FileInputReader reader(&target);
					reader.addData(argv[optind-1]);
			}
			break;

			case 'x': { // hex dump input
					int idx = optind;
					HexbyteInputReader reader(&target);
					while (idx < argc) {
						//std::cout << optind << " " << argv[idx] << endl;
						if (argv[idx][0] == '-') { // next option found
							optind = idx;
							break;
						} else {
							reader.addData(argv[idx]);
						}
						++idx;
					}
				}
				break;
			case 'h':
				usage(argv[0]);
				return false;
		}
	}
	return true;
}


static void
buildCFG(DataSection const &istream)
{
	uint32_t ip = 0; // XXX may actually be different

	Udis86Disassembler dis;
	dis.buffer(istream.getBuffer());

	unsigned bytes;
	while ((bytes = dis.disassemble()) > 0) {
		ip += bytes;
	}
}

int
main(int argc, char **argv)
{
	using namespace std;

	DataSection istream;

	banner();
	if (!parseInputFromOptions(argc, argv, istream))
		exit(1);

	std::cout << "Read " << istream.bytes() << " bytes of input." << std::endl;
	std::cout << "input stream:\n";
	istream.dump();

	std::cout << "---------\n";
	buildCFG(istream);

	return 0;
}
