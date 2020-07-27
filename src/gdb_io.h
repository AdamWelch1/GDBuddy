#ifndef GDB_IO
#define GDB_IO

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <functional>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// #define logPrintf(a, ...) dologPrintf(__FUNCTION__, __LINE__, a,## __VA_ARGS__)

struct ReadBuffer
{
	ReadBuffer()
	{
		numBytes = 0;
		allocSize = 0;
		buf = 0;
	}
	
	uint32_t length() { return numBytes; }
	
	uint32_t numBytes;
	uint32_t allocSize = 0;
	uint8_t *buf;
};

// typedef bool (*gdb_read_func_t)(std::string &);
typedef bool (*gdb_read_func_t)(std::string &);
typedef bool (*gdb_write_func_t)(const std::string &);
typedef void (*commandCallback)(std::string &str);
typedef std::pair<std::string, std::string> KVPair;
typedef std::function<void(std::string &)> GDBCallback;

enum class GDBRecordType : uint8_t
{
	INVALID = 0,
	Result,
	ExecAsync,
	StatusAsync,
	NotifyAsync,
	ConsoleStream,
	TargetStream,
	LogStream
};

struct GDBResponse
{
	GDBRecordType responseType;
	std::string streamText;		// Contains the text of a stream record
	
	std::string recordToken; // Record token-id
	std::string recordClass; // Like result-class, async-class
	std::string recordData;	// Contains record-dependent information
};

struct FileSymbolSet
{
	FileSymbolSet()
	{
		activeFunctionIndex = -1;
	}
	
	std::string fullName;
	std::string shortName;
	int32_t activeFunctionIndex;
	
	// The pair contains a line number and symbol description
	std::vector<std::pair<std::string, std::string>> symbolList;
};

struct DisasLineInfo
{
	std::string addr;
	std::string funcName;
	std::string offset;
	std::string instr;
};

struct FrameInfo
{
	std::string frameAddr;
	std::string frameFunc;
	std::vector<KVPair> frameArgs;
	std::string frameShortname; // Source code file
	std::string frameFullname; // Source code file full path
	std::string frameLine;
	std::string frameArch;
};

struct SignalInfo
{
	std::string signalName;
	std::string signalMeaning;
	std::string signalThread;
	std::string signalThreadStop; // Which threads were stopped
	std::string signalCpuCore;
	FrameInfo 	signalFrame;
};

class GDBIO
{
	public:
	
		GDBIO(gdb_read_func_t r, gdb_write_func_t w);
		~GDBIO();
		
		void setConsole(char *buf, uint32_t bufSize, std::function<uint32_t()> expandFunc)
		{
			m_consoleBuf = buf;
			m_consoleBufUsed = 0;
			m_consoleBufSize = bufSize;
			m_consoleExpandFunc = expandFunc;
		}
		
		void showDisassembly(std::string location)
		{
			disassembleFunc(location, true);
		}
		
		void setVerboseOutput(bool verbose) { m_verboseOutput = verbose; }
		
		bool wasConsoleUpdated()
		{
			if(m_consoleUpdated == true)
			{
				m_consoleUpdated = false;
				return true;
			}
			
			return false;
		}
		
		void loadInferior(std::string inferiorPath);
		void sendCmd(const std::string &str);
		
		void runInferior();
		void contInferior();
		void pauseInferior();
		void execFinInferior(); 		// Execute until return from current stack frame
		void stepOverInferior(); 		// Execute until next source line, don't follow calls
		void stepOverInstInferior(); 	// Execute next instruction, don't follow
		void execRetInferior();			// Return immediately from current stack frame
		void stepInferior(); 			// Execute until next source line, follow calls
		void stepInstInferior(); 		// Execute to next source line, don't follow
		void execUntilInferior(); 		// Execute until [location]
		
		bool inferiorIsRunning() { return m_inferiorIsRunning; }
		bool inferiorExited() { return m_inferiorExited; }
		
		std::string getProjectDir() { return m_projectDir; }
		std::string getStatusStr()
		{
			pthread_mutex_lock(&m_statusMutex);
			std::string ret =  m_statusStr;
			pthread_mutex_unlock(&m_statusMutex);
			return ret;
		}
		
		// Locks mutexes used for data structures displayed in the GUI
		void lockConsole() { pthread_mutex_lock(&m_consoleMutex); }
		void unlockConsole() {pthread_mutex_unlock(&m_consoleMutex); }
		void lockSymbols() { pthread_mutex_lock(&m_symbolsMutex); }
		void unlockSymbols() {pthread_mutex_unlock(&m_symbolsMutex); }
		void lockCodeView() { pthread_mutex_lock(&m_codeViewMutex); }
		void unlockCodeView() {pthread_mutex_unlock(&m_codeViewMutex); }
		
		std::vector<FileSymbolSet> &getFunctionSymbols()
		{ return m_functionSymbols; }
		
		std::vector<FileSymbolSet> &getGlobalVarSymbols()
		{ return m_globalVarSymbols; }
		
		std::vector<DisasLineInfo> &getDisassmemblyLines()
		{ return m_disassemblyLines; }
		
		static void *readThreadThunk(void *param)
		{ return ((GDBIO *) param)->readThread(0); }
		
	private:
	
		void *readThread(void *param); // GDB pipe i/o
		
		// Parses the overall response structure.
		// Extracts the token (if any), determines record type and class,
		// and and then tacks the record-specific data on to be processed
		// in handleResponses().
		// Fills 'out' with GDBResponse objects representing each response
		// that was parsed.
		void parseResponse(std::string &response, std::vector<GDBResponse> &out);
		
		// Handles record-specific data parsing
		void handleResponses(std::vector<GDBResponse> &responseList);
		
		
		void disassembleFunc(std::string location, bool noFlags = false); // Asks GDB for a disassembly dump
		
		void loadFunctionSymbols(); // Requests the data from GDB
		void loadGlobalVarSymbols(); // Requests the data from GDB
		
		// Some helper functions for parsing the GDB/MI output
		std::vector<std::string> parseList(std::string &listStr);
		KVPair getKeyValuePair(std::string &str);
		std::string getTuple(std::string &str);
		
		// Generates a request token to help pair a request with a response
		std::string getRandomToken();
		
		std::vector<FileSymbolSet> 	m_functionSymbols;
		std::vector<FileSymbolSet> 	m_globalVarSymbols;
		std::vector<DisasLineInfo> 	m_disassemblyLines;
		std::vector<SignalInfo>		m_signalList;
		
		FrameInfo m_stopFrameInfo;
		void handleAsyncRecord(GDBResponse &record);
		SignalInfo parseSignal(std::string &signalData);
		void parseFrameData(std::string frameTuple, FrameInfo &frame);
		
		void getFunctionAtAddress(std::string location);
		std::string m_gfaRecvBuffer;
		bool gfaRequestIsPending(std::string &out)
		{
			if(!m_lookupIsPending)
				out = m_gfaRecvBuffer;
				
			return m_lookupIsPending;
		}
		
		bool m_lookupIsPending;
		bool m_exitThreads;
		pthread_t m_readThreadHandle;
		
		std::map<std::string, GDBCallback> m_pendingCommands;
		pthread_mutex_t m_pendingCmdMutex;
		pthread_mutex_t m_statusMutex;		// Lock for m_statusStr
		pthread_mutex_t m_consoleMutex;		// Lock for access to the m_console* variables
		pthread_mutex_t m_symbolsMutex;		// Lock for access to the symbol vectors
		pthread_mutex_t m_codeViewMutex;	// Lock for access to the disassembly and source lines
		pthread_mutex_t m_signalsMutex;		// Lock for access to the signal list
		
		
		void setStatus(const std::string str);
		bool m_inferiorIsRunning;
		bool m_inferiorExited;
		bool m_verboseOutput;
		
		std::string m_statusStr;
		std::string m_projectDir;
		
		void dologPrintf(std::string spec, int32_t line, const char *fmt, ...);
		void logToConsole(const std::string prefix, std::string msg);
		char *m_consoleBuf;
		bool m_consoleUpdated;
		uint32_t m_consoleBufSize;
		uint32_t m_consoleBufUsed;
		std::function<uint32_t()> m_consoleExpandFunc;
		
		gdb_read_func_t 		m_readFunc;
		gdb_write_func_t 		m_writeFunc;
		
		// bool createInferiorProcess();
		int32_t m_inferiorPipeIn[2];
		int32_t m_inferiorPipeOut[2];
		std::string m_inferiorPath;
		pid_t m_inferiorPID;
		
};

#undef logPrintf
#endif
