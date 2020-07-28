#ifndef UNIQUE_GDBMI_LOG_H
#define UNIQUE_GDBMI_LOG_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#endif
	public:
	
		enum class LogLevel : uint8_t
		{
			NeedsFix = 20,
			Error,
			Warn,
			Info,
			Verbose,
			Debug
		};
		
		struct LogItem
		{
			LogLevel logLevel;
			string label;
			string logText;
		};
		
		// printf() compatible log function
		int32_t logPrintf(LogLevel ll, const char *fmt, ...);
		int32_t logPrintf(const char *fmt, ...);
		
		deque<LogItem> getLogs();
		
	private:
	
		void initLogs();
		void destroyLogs();
		
		string getLogLevelColor(LogLevel ll);
		string getLogLevelLabel(LogLevel ll);
		
		deque<LogItem> m_logItems;
		LogLevel m_logLevel;
		mutex m_logMutex;
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


