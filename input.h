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
#pragma once

#include <cassert>		// assert
#include <cstring>		// memset
#include <climits>		// ULONG_MAX
#include <cerrno>		// errno
#include <cstdlib>		// free
#include <cstdio>		// perror
#include <cstdint>

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
	virtual void addData(char const *input) = 0;
	
	/**
	 * @brief Count bytes read
	 *
	 * @return uint32_t
	 **/
	virtual uint32_t bytes() = 0;
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
	{ }

	virtual ~HexbyteInputReader()
	{
		if (_data) {
			free(_data);
		}
	}

	virtual void addData(char const *byte);
	
	/**
	 * @brief Count number of bytes in buffer
	 *
	 * @return uint32_t
	 **/
	virtual uint32_t bytes() { return _data_idx; }

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
	void fit_data();
	
	HexbyteInputReader(HexbyteInputReader const &other)
		: _data(0), _data_idx(0)
	{ }
	HexbyteInputReader& operator= (HexbyteInputReader const& ) { }
};
