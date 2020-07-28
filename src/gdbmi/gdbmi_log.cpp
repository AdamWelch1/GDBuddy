#include "gdbmi_private.h"
#include "gdbmi.h"

#define TERM_RED		"\x1B[31m"
#define TERM_GREEN		"\x1B[32m"
#define TERM_YELLOW		"\x1B[33m"
#define TERM_BLUE		"\x1B[34m"
#define TERM_CYAN		"\x1B[36m"
#define TERM_DEFAULT	"\x1B[0m"

void GDBMI::initLogs()
{
	m_logLevel = GDB_DEFAULT_LOG_LEVEL;
}

void GDBMI::destroyLogs()
{

}

string GDBMI::getLogLevelColor(LogLevel ll)
{
	string ret;
	switch(ll)
	{
		// *INDENT-OFF*
		case LogLevel::NeedsFix: ret = string(TERM_RED); break;
		case LogLevel::Error: ret = string(TERM_RED); break;
		case LogLevel::Warn: ret = string(TERM_YELLOW); break;
		case LogLevel::Info: ret = string(TERM_DEFAULT); break;
		case LogLevel::Verbose: ret = string(TERM_GREEN); break;
		case LogLevel::Debug: ret = string(TERM_CYAN); break;
		// *INDENT-ON*
	}
	
	return ret;
}

int32_t GDBMI::logPrintf(LogLevel ll, const char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	
	char logstr[1024 * 32] = {0};
	int32_t ret = vsprintf(logstr, fmt, v);
	// (1024 * 32 - 8)
	
	va_end(v);
	
	string logLabel = getLogLevelLabel(ll);
	
	LogItem newLog;
	newLog.logLevel = ll;
	newLog.logText = string(logstr);
	newLog.label = logLabel;
	
	m_logMutex.lock();
	m_logItems.push_back(newLog);
	
	while(m_logItems.size() > GDB_MAX_LOG_ITEMS)
		m_logItems.pop_front();
		
	m_logMutex.unlock();
	
	string tColor = getLogLevelColor(newLog.logLevel);
	string out = tColor + logLabel + string(TERM_DEFAULT) + " ";
	out += newLog.logText;
	
	if(out.back() != '\n')
		out.append("\n");
		
	printf("%s", out.c_str());
	
	return ret;
}

// int32_t GDBMI::logPrintf(const char *fmt, ...)
// {
// 	va_list v;
// 	va_start(v, fmt);

// 	char logstr[1024 * 32] = {0};
// 	vsprintf(logstr, fmt, v);
// 	va_end(v);

// 	return logPrintf(LogLevel::NeedsFix, logstr);
// }

deque<GDBMI::LogItem> GDBMI::getLogs()
{
	deque<LogItem> ret;
	m_logMutex.lock();
	ret = m_logItems;
	m_logMutex.unlock();
	
	return ret;
}

string GDBMI::getLogLevelLabel(LogLevel ll)
{
	string ret;
	switch(ll)
	{
	// *INDENT-OFF*
	case LogLevel::NeedsFix: ret = "[FIXME]  "; break;
	case LogLevel::Error: 	 ret = "[Error]  "; break;
	case LogLevel::Warn: 	 ret = "[Warn]   "; break;
	case LogLevel::Info: 	 ret = "[Info]   "; break;
	case LogLevel::Verbose:  ret = "[Verbose]"; break;
	case LogLevel::Debug: 	 ret = "[Debug]  "; break;
	// *INDENT-ON*
	}
	
	return ret;
}
