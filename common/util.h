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
#include <boost/graph/graph_concepts.hpp>

/* GCC defines _GNU_SOURCE, which in turns defines major() and
 * minor() to be macros. We don't need those macros, but want to
 * use the names as identifiers below. Hence, we undefine the
 * macros here.
 */
#undef major
#undef minor

struct Version {
	unsigned major;
	unsigned minor;

	Version(unsigned maj, unsigned min)
		: major(maj), minor(min)
	{ }
};

/**
 * @brief Application configuration, global part
 **/
struct Configuration
{
	bool    verbose;
	bool    debug;
	Version globalProgramVersion;

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
		: verbose(verb), debug(dbg), globalProgramVersion(0,2)
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

struct MessageException
{
	char const *message;
	MessageException(char const *msg)
		: message(msg)
	{ }

	virtual ~MessageException() { }

	MessageException(MessageException const &orig)
		: message(orig.message)
	{ }

	MessageException& operator=(MessageException const &orig)
	{
		message = orig.message;
		return *this;
	}
};

/**
 * @brief Exception indicating that a certain feature has not been implemented yet.
 **/
struct NotImplementedException : public MessageException
{
	NotImplementedException(char const *msg)
		: MessageException(msg)
	{ }
};


struct ThisShouldNeverHappenException : public MessageException
{
	ThisShouldNeverHappenException (const char* msg)
		: MessageException(msg)
	{ }
};


struct FileNotFoundException : public MessageException
{
	FileNotFoundException(char const* file)
		: MessageException(file)
	{ }
};


/**
 * @brief Node with given properties not found in CFG.
 **/
struct NodeNotFoundException { };


/**
 * @brief Error during ELF parsing
 **/
struct ELFException { };
