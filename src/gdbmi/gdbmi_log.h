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
		
		deque<LogItem> getLogs();
		LogLevel getLogLevel();
		void setLogLevel(LogLevel ll);
		
		void logInferiorOutput(string &str);
		deque<string> getInferiorOutput();
		
		void setLogUpdateCB(function<void(GDBMI *)> callback) { m_logUpdateCallback = callback; }
		void setInferiorOutputCB(function<void(GDBMI *)> callback) { m_inferiorOutputCallback = callback; }
		
	private:
	
		void initLogs();
		void destroyLogs();
		
		string getLogLevelColor(LogLevel ll);
		string getLogLevelLabel(LogLevel ll);
		
		deque<LogItem> m_logItems;
		LogLevel m_logLevel;
		mutex m_logMutex;
		
		deque<string> m_inferiorOutput;
		mutex m_inferiorOutMutex;
		
		function<void(GDBMI *)> m_logUpdateCallback = 0;
		function<void(GDBMI *)> m_inferiorOutputCallback = 0;
		
		char *m_logParseBuffer = 0;
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


