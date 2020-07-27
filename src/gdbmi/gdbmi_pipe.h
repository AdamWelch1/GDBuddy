#ifndef UNIQUE_GDBMI_PIPE_H
#define UNIQUE_GDBMI_PIPE_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#define SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_handlers.h"
#undef SOMETHING_UNIQUE_GDBMI_H

#endif

		#ifdef BUILD_GDBMI_TESTS
	public:
		#else
	private:
		#endif
	
		// Called by the GDBMI constructor
		void initPipe();
		
		// Called by the GDBMI destructor
		void destroyPipe();
		
		// void sendCommand(std::string cmd);
		
		static void readThreadThunk(GDBMI *param) { param->readThread(); }
		
		void readThread();
		
		bool readPipe(string &out);
		bool writePipe(string cmd);
		bool runGDB(std::string gdbPath);
		
		thread 		m_readThreadHandle;
		int32_t 	m_gdbPipeIn[2];
		int32_t 	m_gdbPipeOut[2];
		pid_t		m_gdbPID;
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif
