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
	// 2 Mb should be enough. Right? Right???
	m_logParseBuffer = (char *) calloc(1024, 2048);
	m_logLevel = GDB_DEFAULT_LOG_LEVEL;
}

void GDBMI::destroyLogs()
{
	if(m_logParseBuffer != 0)
		free(m_logParseBuffer);
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
	
	char *logstr = m_logParseBuffer;
	int32_t ret = vsnprintf(logstr, (1024 * 2048), fmt, v);
	va_end(v);
	
	if(ll > getLogLevel())
		return 0;
		
	string logLabel = getLogLevelLabel(ll);
	string tColor = getLogLevelColor(ll);
	string out = tColor + logLabel + string(TERM_DEFAULT) + " ";
	
	LogItem newLog;
	newLog.logLevel = ll;
	newLog.logText = string(logstr);
	newLog.label = logLabel;
	
	// This keeps us from dumping a bazillion lines to the console
	if(newLog.logText.length() > 1000)
		newLog.logText = newLog.logText.substr(0, 1000) + "--CUT--\n";
		
	out += newLog.logText;
	m_logMutex.lock();
	m_logItems.push_back(newLog);
	
	while(m_logItems.size() > GDB_MAX_LOG_ITEMS)
		m_logItems.pop_front();
		
	if(out.back() != '\n')
		out.append("\n");
		
	printf("%s", out.c_str());
	m_logMutex.unlock();
	
	if(m_logUpdateCallback != 0)
		m_logUpdateCallback(this);
		
	return ret;
}

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

GDBMI::LogLevel GDBMI::getLogLevel()
{
	LogLevel ret;
	m_logMutex.lock();
	ret = m_logLevel;
	m_logMutex.unlock();
	
	return ret;
}

void GDBMI::setLogLevel(LogLevel ll)
{
	m_logMutex.lock();
	m_logLevel = ll;
	m_logMutex.unlock();
}

void GDBMI::logInferiorOutput(string &str)
{
	m_inferiorOutMutex.lock();
	m_inferiorOutput.push_back(str);
	m_inferiorOutMutex.unlock();
	
	if(m_inferiorOutputCallback != 0)
		m_inferiorOutputCallback(this);
}

deque<string> GDBMI::getInferiorOutput()
{
	deque<string> ret;
	m_inferiorOutMutex.lock();
	ret = m_inferiorOutput;
	m_inferiorOutMutex.unlock();
	
	return ret;
}
