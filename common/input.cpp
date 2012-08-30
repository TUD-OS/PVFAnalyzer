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
#include "data/input.h"
#include "util.h"

#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>

#include <boost/foreach.hpp>


/***********************************************************************
 *                                                                     *
 *                             INPUT STREAM                            *
 *                                                                     *
 ***********************************************************************/


void DataSection::addByte(uint8_t byte)
{
	fitData();
	_data[_dataIndex++] = byte;
}


void DataSection::addBytes(uint8_t* buf, size_t count)
{
	(void)buf;
	fitData(count);
	assert(false);
}


void DataSection::dump()
{
	std::cout << "--- Dump of section '" << name() << "' ---" << std::endl;

	if (!_data) {
		std::cout << "<empty stream>" << std::endl;
		return;
	}

	for (unsigned i = 0; i < _dataIndex; ++i) {
		std::cout << std::hex << (uint32_t)_data[i] << " ";
		if ((i % 30 == 29)) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}


void
DataSection::fitData(size_t howmuch)
{
	/*
	 * We only alloc if a) no data has been allocated yet or b) we have
	 * less than <howmuch> bytes left. If we need to alloc a new chunk, we always
	 * increase the data chunk size by DATA_INCREMENT bytes.
	 */
	if ( ( _dataIndex > 0 ) and ( _dataIndex % DATA_INCREMENT < (DATA_INCREMENT-howmuch-1) ) )
		return;

	/* The (+ DATA_INCREMENT) is to get the actual number of allocated chunks. The (-1) is
	 * to make sure that the first time we run through here, how_many_chunks is 0.
	 */
	unsigned how_many_chunks = ( ( _dataIndex + DATA_INCREMENT - 1 ) / DATA_INCREMENT );
	unsigned newsize = how_many_chunks * DATA_INCREMENT + DATA_INCREMENT;

	_data = static_cast<uint8_t*> ( realloc ( _data, newsize ) );
	assert ( _data );

	memset ( _data + _dataIndex, 0, newsize - _dataIndex );
}


/***********************************************************************
 *                                                                     *
 *                             HEX BYTE READER                         *
 *                                                                     *
 ***********************************************************************/


void
HexbyteInputReader::addData(const char* byte)
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
FileInputReader::isELFFile(std::ifstream& str)
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
FileInputReader::addData(const char* filename)
{
	std::ifstream ifs;

	ifs.open(filename, std::ios::in | std::ios::binary);

	if (!ifs.is_open()) {
		return;
	}

	if (isELFFile(ifs)) {
		parseElf(filename);
	} else {
		// single input section, again...
		_sections.push_back(new DataSection());
		std::string secname = "binary: ";
		secname += filename;
		section(0)->name(secname);
		ifs.seekg(0, std::ios::beg);
		do {
			char c[1];
			ifs.read(c, 1);
			if (!ifs.fail())
				section(0)->addByte(static_cast<uint8_t>(c[0]));
		} while (!ifs.eof());
	}
}

static char const *
ptype_str(size_t pt)
{
	switch(pt) {
		case PT_NULL:      return "NULL";
		case PT_LOAD:      return "LOAD";
		case PT_DYNAMIC:   return "DYNAMIC";
		case PT_INTERP:    return "INTERP";
		case PT_NOTE:      return "NOTE";
		case PT_SHLIB:     return "SHLIB";
		case PT_PHDR:      return "PHDR";
		case PT_TLS:       return "TLS";
		case PT_SUNWBSS:   return "SUNWBSS";
		case PT_SUNWSTACK: return "STACK";
		default:           return "unknown";
	}
}

void FileInputReader::parseElf(const char* filename)
{
	Elf* elf;
	int elffd;
	GElf_Ehdr ehdr;
	GElf_Phdr phdr;

	elffd = openElf(filename, &elf);

	if (gelf_getehdr(elf, &ehdr) == 0) {
		std::cout << "Could not getehdr()" << std::endl;
		throw ELFException();
	}
	DEBUG(std::cout << "Entry: " << std::hex << ehdr.e_entry << std::endl;);
	_entry = Address(ehdr.e_entry);

	size_t shnum, phnum;
	if (elf_getshdrnum(elf, &shnum) != 0) {
		std::cout << "getshdrnum() failed" << std::endl;
		throw ELFException();
	}

	if (elf_getphdrnum(elf, &phnum) != 0) {
		std::cout << "getphdrnum() failed" << std::endl;
		throw ELFException();
	}

	DEBUG(std::cout << "Number of section headers: " << shnum << std::endl;);
	DEBUG(std::cout << "Number of program headers: " << phnum << std::endl;);

	for (unsigned i = 0; i < phnum; ++i) {
		if (gelf_getphdr(elf, i, &phdr) != &phdr) {
			std::cout << "Get PHDR [" << i << "] failed." << std::endl;
			throw ELFException();
		}
		if (Configuration::get()->debug) {
			std::cout << "PHDR " << i << ":" <<  std::endl;
			dumpElfPhdr(phdr);
		}

		if (phdr.p_type == PT_LOAD) {
			loadElfPhdr(elffd, phdr);
		}
	}

	if (Configuration::get()->verbose) {
		BOOST_FOREACH(DataSection *ds, _sections) {
				RelocatedMemRegion mr = ds->getBuffer();
				std::cout << "Section mem [";
				std::cout << mr.base.v << " - " << mr.base.v + mr.size;
				std::cout << "], reloc [" << mr.mappedBase.v << " - ";
				std::cout << mr.mappedBase.v + mr.size << "]" << std::endl;
		}
	}

	parseSections(elf);

	elf_end(elf);
	close(elffd);
}


void FileInputReader::loadElfPhdr(int elffd, GElf_Phdr& phdr)
{
	uint8_t *buffer;
	DEBUG(std::cout << "Allocating mem buffer. Size " << phdr.p_memsz
	                << ", alignment " << phdr.p_align << std::endl;);
	int fail = posix_memalign((void**)&buffer, phdr.p_align, phdr.p_memsz);
	if (fail) {
		std::cout << "Aligned memory allocation failed: " << fail << std::endl;
		throw ELFException();
	}
	DEBUG(std::cout << "Success: " << std::hex << (void*)buffer << std::endl;);

	memset(buffer, 0, phdr.p_memsz);
	ssize_t bytes = pread(elffd, buffer, phdr.p_filesz, phdr.p_offset);
	if (bytes != (ssize_t)phdr.p_filesz) {
		std::cout << "pread() = " << bytes << ", but should be " << phdr.p_filesz << std::endl;
		throw ELFException();
	}

	_sections.push_back(new DataSection());
	DataSection* ds = section(_sections.size()-1);
	ds->addBuffer(buffer, phdr.p_memsz);
	ds->relocationAddress(Address(phdr.p_vaddr));
	DEBUG(std::cout << (void*)buffer << " -> " << phdr.p_vaddr << std::endl;);
}


void FileInputReader::dumpElfPhdr(GElf_Phdr& phdr)
{
	std::cout << "       type  " << std::hex << std::setw(9) << ptype_str(phdr.p_type) << std::endl;
	std::cout << "     offset  " << std::hex << std::setw(9) << phdr.p_offset << std::endl;
	std::cout << "      vaddr  " << std::hex << std::setw(9) << phdr.p_vaddr << std::endl;
	std::cout << "      paddr  " << std::hex << std::setw(9) << phdr.p_paddr << std::endl;
	std::cout << "    memsize  " << std::hex << std::setw(9) << phdr.p_memsz << std::endl;
	std::cout << "   filesize  " << std::hex << std::setw(9) << phdr.p_filesz << std::endl;
	std::cout << "      flags  " << phdr.p_flags << " ( ";
	if (phdr.p_flags & PF_X) std::cout << "exec ";
	if (phdr.p_flags & PF_R) std::cout << "read ";
	if (phdr.p_flags & PF_W) std::cout << "write";
	std::cout << " )" << std::endl;
	std::cout << "      align  " << std::hex << std::setw(9) << phdr.p_align << std::endl;
}


int FileInputReader::openElf(char const *filename, Elf** elf)
{
	int fd;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		std::cout << "Could not initialize ELF library." << std::endl;
		throw ELFException();
	}

	if ((fd = open(filename, O_RDONLY, 0)) < 0) {
		perror("file open");
		throw ELFException();
	}

	if ((*elf = elf_begin(fd, ELF_C_READ, 0)) == 0) {
		std::cout << "elf_begin failed." << std::endl;
		throw ELFException();
	}

	if (elf_kind(*elf) != ELF_K_ELF) {
		std::cout << "not an ELF object!" << std::endl;
		throw ELFException();
	}

	return fd;
}


bool
FileInputReader::insideJumpTable(Address const &a)
{
	return false;
}


void FileInputReader::parseSections(Elf *elf)
{
	Elf_Scn *section;
	char *name;
	size_t shstrndx;

	if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
		std::cout << "get section header string index failed:";
		std::cout << elf_errmsg(-1) << std::endl;
		throw ELFException();
	}

	section = 0;
	while ((section = elf_nextscn(elf, section)) != 0) {
		GElf_Shdr hdr;
		if (gelf_getshdr(section, &hdr) != &hdr) {
			std::cout << "Could not get section header: " << elf_errmsg(-1) << std::endl;
			throw ELFException();
		}

		if ((name = elf_strptr(elf, shstrndx, hdr.sh_name)) == 0) {
			std::cout << "No section name? " << elf_errmsg(-1) << std::endl;
			throw ELFException();
		}

		DEBUG(std::cout << "Section name: " << std::setw(20) << name
		                << "    " << std::hex << "@ 0x" << hdr.sh_addr << " sz="
		                << hdr.sh_size << std::endl;);
	}
}