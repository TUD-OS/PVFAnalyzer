#include "input.h"

void HexbyteInputReader::addData ( const char* byte )
{
	fit_data();

	/* error handling taken straight from the strol() manpage */
	errno = 0;
	char *end = 0;
	uint32_t data = strtoul(byte, &end, 16);

	//printf("errno %d data %2llx, %p-%p\n", errno, data, byte, end);
	if ((errno == ERANGE and 
			(data == ULONG_MAX or data == 0))
		or (errno != 0 and data == 0)
		or (end - byte != 2)) {
		//std::perror("strtol");
		return;
	}

	_data[_data_idx++] = data & 0xFF;
}

void HexbyteInputReader::fit_data()
{
	/*
	 * We only alloc if a) no data has been allocated yet or b) we have
	 * less than 2 bytes left. If we need to alloc a new chunk, we always
	 * increase the data chunk size by DATA_INCREMENT bytes.
	 */
	if ( ( _data_idx > 0 ) and ( _data_idx % DATA_INCREMENT < DATA_INCREMENT-2 ) )
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
