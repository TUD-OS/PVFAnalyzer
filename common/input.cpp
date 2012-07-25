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
#include "input.h"

#include <iostream>

InputReader::~InputReader()
{ }


/***********************************************************************
 *                                                                     *
 *                             INPUT STREAM                            *
 *                                                                     *
 ***********************************************************************/


void DataSection::addByte ( uint8_t byte )
{
	fit_data();
	_data[_data_idx++] = byte;
}


void DataSection::addBytes ( uint8_t* buf, size_t count )
{
	(void)buf;
	fit_data(count);
	assert(false);
}


void DataSection::dump()
{
	if (!_data) {
		std::cout << "<empty stream>" << std::endl;
		return;
	}

	for (unsigned i = 0; i < _data_idx; ++i) {
		std::cout << std::hex << (uint32_t)_data[i] << " ";
		if ((i % 30 == 29)) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}


void
DataSection::fit_data(size_t howmuch)
{
	/*
	 * We only alloc if a) no data has been allocated yet or b) we have
	 * less than <howmuch> bytes left. If we need to alloc a new chunk, we always
	 * increase the data chunk size by DATA_INCREMENT bytes.
	 */
	if ( ( _data_idx > 0 ) and ( _data_idx % DATA_INCREMENT < (DATA_INCREMENT-howmuch-1) ) )
		return;

	/* The (+ DATA_INCREMENT) is to get the actual number of allocated chunks. The (-1) is
	 * to make sure that the first time we run through here, how_many_chunks is 0.
	 */
	unsigned how_many_chunks = ( ( _data_idx + DATA_INCREMENT - 1 ) / DATA_INCREMENT );
	unsigned newsize = how_many_chunks * DATA_INCREMENT + DATA_INCREMENT;

	_data = static_cast<uint8_t*> ( realloc ( _data, newsize ) );
	assert ( _data );

	memset ( _data + _data_idx, 0, newsize - _data_idx );
}


/***********************************************************************
 *                                                                     *
 *                             HEX BYTE READER                         *
 *                                                                     *
 ***********************************************************************/


void
HexbyteInputReader::addData ( const char* byte )
{
	/* error handling taken straight from the strol() manpage */
	errno = 0;
	char *end = 0;
	uint64_t data = strtoul(byte, &end, 16);

	//printf("errno %d data %2llx, %p-%p\n", errno, data, byte, end);
	if ((errno == ERANGE and 
			(data == ULONG_MAX or data == 0))
		or (errno != 0 and data == 0)
		or (end - byte != 2)) {
		//std::perror("strtol");
		return;
	}

	section(0)->addByte(data & 0xFF);
}

#include <libelf.h>

/***********************************************************************
 *                                                                     *
 *                             FILE READER                             *
 *                                                                     *
 ***********************************************************************/

bool
FileInputReader::is_elf_file ( std::ifstream& str )
{
	char first_bytes[4];
	str.read(first_bytes, sizeof(first_bytes));

	if (str.gcount() != 4) // not enough characters for ELF magic
		return false;

	if ( (first_bytes[0] == ELFMAG0) and
	     (first_bytes[1] == ELFMAG1) and
	     (first_bytes[2] == ELFMAG2) and
	     (first_bytes[3] == ELFMAG3))
		return true;

	return false;
}

void
FileInputReader::addData (const char* filename)
{
	std::ifstream ifs;

	ifs.open(filename, std::ios::in | std::ios::binary);

	if (!ifs.is_open()) {
		return;
	}

	if (is_elf_file(ifs)) {
		std::cout << "ELF parsing not implemented yet." << std::endl;
	} else {
		// single input section, again...
		_sections.push_back(DataSection());
		ifs.seekg(0, std::ios::beg);
		do {
			char c[1];
			ifs.read(c, 1);
			if (!ifs.fail())
				section(0)->addByte(static_cast<uint8_t>(c[0]));
		} while (!ifs.eof());
	}
}
