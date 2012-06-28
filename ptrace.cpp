#include <iostream>

#include "tracer.h"

Tracer::~Tracer()
{
}


void PTracer::run()
{
	std::cout << __FUNCTION__ << std::endl;
}

