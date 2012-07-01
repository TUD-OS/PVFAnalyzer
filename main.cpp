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
#include <cassert>		// assert
#include <cstring>		// memset
#include <cstdio>		// perror
#include <cerrno>		// errno
#include <climits>		// ULONG_MAX

#include <getopt.h>		// getopt()

/**
 * @brief Command line long options
 **/
struct option my_opts[] = {
	{"hex", no_argument, 0, 'x'},
	{0,0,0,0} // this line be last
};

/**
 * @brief Generic interface of an input reader.
 **/
class InputReader
{
public:
	virtual ~InputReader() { }

	/**
	 * @brief Read input
	 *
	 * The input comes from the command line and its interpretation depends
	 * on the concrete implementation of the input reader. The argument may
	 * be a string on the command line, a filename to open, a URL to download
	 * from or whatever else may suit.
	 *
	 * @param input input
	 **/
	virtual void addData(char *input) = 0;
};

/**
 * @brief Reader for hexadecimal bytes from the command line
 *
 * Input consisting of two-digit hexadecimal numbers is read directly from the
 * command line and stored in a buffer.
 *
 * So, the command line string "AB CD EF 12 34 56" will end up in a buffer
 * containing 7 bytes:
 *
 * 0xAB 0xCD 0xEF 0x12 0x34 0x 56
 **/
class HexbyteInputReader : public InputReader
{
public:
	HexbyteInputReader()
		: _data(0), _data_idx(0)
	{
	}

	virtual ~HexbyteInputReader()
	{
		if (_data) free(_data);
	}

	virtual void addData(char *byte)
	{
		fit_data();
		
		/* error handling taken straight from the strol() manpage */
		errno = 0;
		uint64_t data = strtoul(byte, 0, 16);
		
		if ((errno == ERANGE and 
				(data == ULONG_MAX or data == 0))
            or (errno != 0 and data == 0)) {
			std::perror("strtol");
			throw 0; // FIXME: error handling/reporting?
		}
		
		_data[_data_idx++] = data & 0xFF;
	}
	
	/**
	 * @brief Count number of bytes in buffer
	 *
	 * @return uint32_t
	 **/
	uint32_t bytes() { return _data_idx; }

private:
	enum { DATA_INCREMENT = 1024, };

	uint8_t* _data;
	unsigned _data_idx;

	/**
	 * @brief Make sure the available data area is big enough
	 *
	 * Make sure that the input buffer is big enough to read in at
	 * least one more chunk of input data.
	 **/
	void fit_data()
	{
		/*
		 * We only alloc if a) no data has been allocated yet or b) we have
		 * less than 2 bytes left. If we need to alloc a new chunk, we always
		 * increase the data chunk size by DATA_INCREMENT bytes.
		 */
		if ((_data_idx > 0) and (_data_idx % DATA_INCREMENT < DATA_INCREMENT-2))
			return;

		/* The (+ DATA_INCREMENT) is to get the actual number of allocated chunks. The (-1) is
		 * to make sure that the first time we run through here, how_many_chunks is 0.
		 */
		unsigned how_many_chunks = ((_data_idx + DATA_INCREMENT - 1) / DATA_INCREMENT);
		unsigned newsize = how_many_chunks * DATA_INCREMENT + DATA_INCREMENT;

		_data = static_cast<uint8_t*>(realloc(_data, newsize));
		assert(_data);

		memset(_data + _data_idx, 0, newsize - _data_idx);
	}
};

int main(int argc, char **argv)
{
	int opt;
	using namespace std;
	
	cout << "hello" << endl;
	
	while ((opt = getopt(argc, argv, "x")) != -1) {
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
		}
	}

	return 0;
}
