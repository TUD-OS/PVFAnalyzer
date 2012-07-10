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

#include <fstream>
#include <iostream>

/**
 * @brief Representation of an raw data buffer
 **/
class RawData
{
public:
	RawData()
		: _data(0), _data_idx(0)
	{
	}

	~RawData()
	{
		if (_data) {
			free(_data);
		}
	}

	/**
	 * @brief Add a single byte to the buffer.
	 *
	 * @param byte byte to add
	 * @return void
	 **/
	void addByte(uint8_t byte);

	/**
	 * @brief Add multiple bytes at once
	 *
	 * @param buf buffer containing bytes to add
	 * @param count number of bytes to add from buffer
	 * @return void
	 **/
	void addBytes(uint8_t* buf, size_t count);


	/**
	 * @brief Get pointer to bytes at offset
	 *
	 * @param index offset
	 * @return uint8_t* const	pointer into buffer if offset fits
	 *                         into buf, NULL otherwise
	 **/
	uint8_t const * const getPtr(uint32_t offset) const
	{
		if (offset <= _data_idx) {
			return _data + offset;
		} else {
			return 0;
		}
	}

	/**
	 * @brief Dump buffer content to stdout
	 *
	 * @return void
	 **/
	void dump();

	/**
	 * @brief Get number of bytes that are stored in the stream.
	 *
	 * @return uint32_t number of bytes
	 **/
	uint32_t bytes() { return _data_idx; }

private:
	enum { DATA_INCREMENT = 1024, };

	uint8_t* _data;			// buffer ptr
	uint32_t _data_idx;		// next idx to write to

	RawData(const RawData&)
		: _data(0), _data_idx(0)
	{ }

	RawData& operator=(RawData &) { return *this; }

	/**
	 * @brief Make sure the available data area is big enough
	 *
	 * Make sure that the input buffer is big enough to read in at
	 * least one more chunk of input data.
	 **/
	void fit_data(size_t count = 1);
};


/**
 * @brief Generic interface of an input reader.
 *
 * An input reader reads input from a source and streams the bytes
 * found at the source into an InputStream object.
 **/
class InputReader
{
protected:
	RawData *_the_stream;
public:
	InputReader(RawData *is = 0)
		: _the_stream(is)
	{ }

	virtual ~InputReader();

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
private:
	InputReader(InputReader const&) : _the_stream(0) { }
	InputReader& operator=(const InputReader&) { return *this; }
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
	HexbyteInputReader(RawData *istream)
		: InputReader(istream)
	{ }

	virtual ~HexbyteInputReader()
	{ }

	virtual void addData(char const *byte);

private:
	
	HexbyteInputReader(HexbyteInputReader const &) { }
	HexbyteInputReader& operator= (HexbyteInputReader const& ) { return *this; }
};


class FileInputReader : public InputReader
{
public:
	FileInputReader(RawData *istream)
		: InputReader(istream)
	{ }

	virtual void addData(char const *file);

private:
	FileInputReader(FileInputReader const&)
	{ }

	FileInputReader& operator= (FileInputReader const &) { return *this; }

	/**
	 * @brief Determine if the input file stream refers to an ELF binary.
	 *
	 * @param str input stream
	 * @return bool true if ELF binary, false otherwise
	 **/
	bool is_elf_file(std::ifstream& str);
};
