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

#ifndef TRACER_H
#define TRACER_H

#include <string>

/**
 * @brief Base class for all dynamic analysis components.
 **/
class Tracer
{

public:
	Tracer() { }
	virtual ~Tracer();

	/**
	 * @brief Start an analysis run.
	 *
	 * @return void
	 **/
	virtual void run() = 0;

protected:
	
	Tracer(const Tracer&) { }
	virtual Tracer& operator=(const Tracer& other) = 0;
};



/**
 * @brief PTrace-based analyzer for Linux binaries
 **/
class PTracer : public Tracer
{
public:
	virtual ~PTracer();
	
	virtual void run();
	
protected:
	virtual PTracer& operator=(const PTracer& other) { (void)other; return *this; }
};

#endif // TRACER_H
