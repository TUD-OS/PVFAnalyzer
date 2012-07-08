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
	{"help", no_argument, 0, 'h' },
	{"hex", no_argument, 0, 'x'},
	{0,0,0,0} // this line be last
};


int main(int argc, char **argv)
{
	int opt;
	using namespace std;
	
	cout << "hello" << endl;
	
	while ((opt = getopt(argc, argv, "hx")) != -1) {
		switch(opt) {
			case 'x': { // hex dump input
					int idx = optind;
					while (idx < argc) {
						std::cout << optind << " " << argv[idx] << endl;
						if (argv[idx][0] == '-') { // next option found
							optind = idx;
							break;
						} else {
						}
						++idx;
					}
				}
				break;
			case 'h': { // help
				printf("\033[32mUsage:\033[0m\n\n");
				printf("%s [-h] [-x <bytestream>]\n\n\033[32mOptions\033[0m\n", argv[0]);
				printf("\t-h                 Display help\n");
				printf("\t-x <bytes>         Interpret the following two-digit hexadecimal numbers\n");
				printf("\t                   as input to work on.\n");
			}
		}
	}

	return 0;
}
