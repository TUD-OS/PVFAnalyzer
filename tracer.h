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

class Tracer
{

public:
	Tracer(std::string& filename);
	Tracer(const Tracer& other);
	virtual ~Tracer();
	
	virtual void run() = 0;
	
	std::string const & filename() { return _filename; }

protected:
	std::string const _filename;

	virtual Tracer& operator=(const Tracer& other) = 0;
};



class PTracer : public Tracer
{
public:
	PTracer(std::string& filename)
		: Tracer(filename)
	{	}
	
	virtual ~PTracer();
	
	virtual void run();
	
protected:
	virtual PTracer& operator=(const PTracer& other) { (void)other; return *this; }
};

#endif // TRACER_H
