#include "input.h"

#include <iostream>

InputReader::~InputReader()
{ }


/***********************************************************************
 *                                                                     *
 *                             INPUT STREAM                            *
 *                                                                     *
 ***********************************************************************/


void InputStream::addByte ( uint8_t byte )
{
	fit_data();
	_data[_data_idx++] = byte;
}


void InputStream::addBytes ( uint8_t* buf, size_t count )
{
	(void)buf;
	fit_data(count);
	assert(false);
}


void InputStream::dump()
{
	if (!_data) {
		std::cout << "<empty stream>" << std::endl;
		return;
	}

	for (unsigned i = 0; i < _data_idx; ++i) {
		std::cout << std::hex << (uint32_t)_data[i] << " ";
		if ((i % 30 == 29) or (i == _data_idx)) {
			std::cout << std::endl;
		}
	}
}


void
InputStream::fit_data(size_t howmuch)
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

	_the_stream->addByte(data & 0xFF);
}


/***********************************************************************
 *                                                                     *
 *                             FILE READER                             *
 *                                                                     *
 ***********************************************************************/

void
FileInputReader::addData (const char* filename)
{

}
