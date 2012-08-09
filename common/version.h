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

	Configuration()
		: verbose(false), debug(false), global_program_version(0,0)
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
