#include "gdbmi.h"

GDBMI::GDBMI()
{
	m_exitThreads = false;
	
	initPipe();
	sleep(1); // This is to give time for the forked child to call exec(), otherwise bad things happen :(
	initHandlers();
	
	sendCommand("-gdb-set mi-async on");
}

GDBMI::~GDBMI()
{
	fprintf(stderr, "destructor!\n");
	m_exitThreads = true;
	
	destroyHandlers();
	destroyPipe();
}
