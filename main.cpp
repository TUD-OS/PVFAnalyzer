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
#include "input.h"      // InputReader

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
	std::cout << prog << " [-h] [-x <bytestream>]"
	          << std::endl << std::endl << "033[32mOptions\033[0m" << std::endl;
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


int
main(int argc, char **argv)
{
	int opt;
	using namespace std;

	InputStream istream;
	InputReader *reader = 0;

	banner();

	while ((opt = getopt(argc, argv, "f:hx")) != -1) {
		switch(opt) {
			case 'f': { // file input
					if (reader) {
						std::cout << "\033[31m" << "Error: multiple input streams selected."
						          << "\033[0m"  << std::endl;
					}
					std::cout << "input file: " << argv[optind-1] << std::endl;
					reader = new FileInputReader(&istream, argv[optind-1]);
			}
			break;

			case 'x': { // hex dump input
					int idx = optind;
					if (reader) {
						std::cout << "\033[31m" << "Error: multiple input streams selected."
						          << "\033[0m"  << std::endl;
					}
					reader = new HexbyteInputReader(&istream);
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
				}
				break;
			case 'h':
				usage(argv[0]);
				return 0;
		}
	}

	std::cout << "input stream:\n";
	istream.dump();

	return 0;
}
