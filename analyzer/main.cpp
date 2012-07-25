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

#include <iostream>	    // std::cout
#include <getopt.h>	    // getopt()
#include "input.h"     // DataSection, InputReader
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
parseInputFromOptions(int argc, char **argv, std::vector<InputReader*>& retvec)
{
	int opt;

	while ((opt = getopt(argc, argv, "f:hx")) != -1) {
		switch(opt) {
			case 'f': { // file input
					std::cout << "input file: " << argv[optind-1] << std::endl;
					FileInputReader *fr = new FileInputReader();
					retvec.push_back(fr);
					fr->addData(argv[optind-1]);
			}
			break;

			case 'x': { // hex dump input
					int idx = optind;
					HexbyteInputReader *reader = new HexbyteInputReader();
					while (idx < argc) {
						//std::cout << optind << " " << argv[idx] << endl;
						if (argv[idx][0] == '-') { // next option found
							optind = idx;
							break;
						} else {
							reader->addData(argv[idx]);
						}
						++idx;
					}
					retvec.push_back(reader);
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
buildCFG(std::vector<InputReader*> const & v)
{
	Udis86Disassembler dis;

	for (std::vector<InputReader*>::const_iterator it = v.begin();
		 it != v.end(); ++it) {
		for (unsigned sec = 0; sec < (*it)->section_count(); ++sec) {
			dis.buffer((*it)->section(sec)->getBuffer());

			Address ip     = 0; // XXX may actually be different
			unsigned bytes = 0;

			while ((bytes = dis.disassemble()) > 0) {
				ip += bytes;
			}
		}
	}
}


static unsigned count_bytes(std::vector<InputReader*> const & rv)
{
	unsigned bytes = 0;
	for (std::vector<InputReader*>::const_iterator it = rv.begin();
		 it != rv.end(); ++it) {
		for (unsigned sec = 0; sec < (*it)->section_count(); ++sec) {
			bytes += (*it)->section(sec)->bytes();
		}
	}

	return bytes;
}


static void dump_sections(std::vector<InputReader*> const & rv)
{
	for (std::vector<InputReader*>::const_iterator it = rv.begin();
		 it != rv.end(); ++it) {
		for (unsigned sec = 0; sec < (*it)->section_count(); ++sec) {
			(*it)->section(sec)->dump();
		}
	}
}

int
main(int argc, char **argv)
{
	using namespace std;

	std::vector<InputReader*> input;

	if (not parseInputFromOptions(argc, argv, input))
		exit(2);

	banner();
	if (input.size() == 0)
		exit(1);

	std::cout << "Read " << count_bytes(input) << " bytes of input." << std::endl;
	std::cout << "input stream:\n";
	dump_sections(input);

	std::cout << "---------\n";
	buildCFG(input);

	return 0;
}
