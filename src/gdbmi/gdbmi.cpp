#include "gdbmi.h"

GDBMI::GDBMI()
{
	m_exitThreads = false;
	
	initLogs();
	initPipe();
	sleep(1); // This is to give time for the forked child to call exec(), otherwise bad things happen :(
	initHandlers();
	initState();
	initControl();
	
	sendCommand("-gdb-set mi-async on");
	sendCommand("-gdb-set disassembly-flavor intel");
}

GDBMI::~GDBMI()
{
	m_exitThreads = true;
	
	destroyControl();
	destroyState();
	destroyHandlers();
	destroyPipe();
	destroyLogs();
}
