#ifndef UNIQUE_GDBMI_STATE_H
#define UNIQUE_GDBMI_STATE_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{
#endif

	public:
	
		enum class GDBState : uint8_t
		{
			Running = 50,
			Stopped,
			Exited
		};
		
		// Called by the GDBMI constructor
		void initState();
		
		// Called by the GDBMI destructor
		void destroyState();
		
		void setState(GDBState state, const string msg);
		GDBState getState();
		string getStatusMsg();
		
	private:
		GDBState m_gdbState;
		string m_stopMsg;
		mutex m_stateMutex;
		
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


