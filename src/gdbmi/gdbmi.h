#ifndef SOMETHING_UNIQUE_GDBMI_H
#define SOMETHING_UNIQUE_GDBMI_H

#include <cstdint>
#include <cstdarg>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <deque>
#include <thread>
#include <mutex>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>

#define GDB_HANDLER_THREAD_COUNT	32
#define GDB_DEFAULT_LOG_LEVEL		LogLevel::Debug
#define GDB_MAX_LOG_ITEMS			1024

using std::string;
using std::vector;
using std::pair;
using std::deque;
using std::thread;
using std::mutex;

typedef string 					ListItem;
typedef vector<ListItem> 		ListItemVector;
typedef pair<string, string> 	KVPair;
typedef vector<KVPair>			KVPairVector;

class GDBMI
{
	public:
	
		GDBMI();
		~GDBMI();
		
	// *INDENT-OFF*
	#include "gdbmi_log.h"
	#include "gdbmi_parse.h"
	#include "gdbmi_handlers.h"
	#include "gdbmi_pipe.h"
	#include "gdbmi_control.h"
	#include "gdbmi_state.h"
	#include "gdbmi_data.h"
	// *INDENT-ON*
		
		bool m_exitThreads;
		
};



#endif
