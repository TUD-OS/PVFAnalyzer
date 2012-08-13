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

#include <cassert>

struct version_t {
	unsigned _major;
	unsigned _minor;

	version_t(unsigned maj, unsigned min)
		: _major(maj), _minor(min)
	{ }

	unsigned major() { return _major; }
	unsigned minor() { return _minor; }
};

struct Configuration
{
	bool      verbose;
	bool      debug;
	version_t global_program_version;

	bool parse_option(int opt)
	{
		switch(opt) {
			case 'd':
				debug = true;
				return true;
			case 'v':
				verbose = true;
				return true;
			default:
				return false;
		}
	}

	Configuration(bool verb = false, bool dbg = false)
		: verbose(verb), debug(dbg), global_program_version(0,0)
	{ }

	virtual ~Configuration() { }

private:
	static Configuration* globalConfig;

public:
	static void setConfig(Configuration* c) { globalConfig = c; }
	static Configuration* get()             { assert(globalConfig); return globalConfig; }
};

#define VERBOSE(x) \
	do { if (Configuration::get()->verbose) { x } } while (0)

#define DEBUG(x) \
	do { if (Configuration::get()->debug) { std::cout << "DBG: [" << __func__ << "] "; { x }} } while (0);


/* My exceptions */
struct NotImplementedException
{
	char const *message;
	NotImplementedException(char const *msg)
		: message(msg)
	{ }
};

struct NodeNotFoundException
{
};