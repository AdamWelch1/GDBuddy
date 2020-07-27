#ifndef UNIQUE_GDBMI_HANDLERS_H
#define UNIQUE_GDBMI_HANDLERS_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#define SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_parse.h"
#undef SOMETHING_UNIQUE_GDBMI_H

#endif

		typedef std::function<void(std::string &)> CmdCallback;
		typedef std::map<string, CmdCallback>::iterator CallbackIter;
		
		#ifdef BUILD_GDBMI_TESTS
	public:
		#else
	private:
		#endif
	
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
			GDBRecordType recordType;
			std::string recordToken; // Record token-id
			std::string recordClass; // Like result-class, async-class
			std::string recordData;	// Contains record-dependent information
		};
		
		// Called by the GDBMI constructor
		void initHandlers();
		
		// Called by the GDBMI destructor
		void destroyHandlers();
		
		// This handler identifies the type of record given in responseStr
		// and creates a GDBResponse object containing the response token,
		// record type, and record class. The record-specific data is put in
		// recordData.
		// handleResponse() Then hands off the GDBResponse object to the
		// handler for the given record type.
		void handleResponse(string &responseStr);
		
		void handleResultRecord(GDBResponse response);
		void handleExecAsyncRecord(GDBResponse response);
		void handleStatusAsyncRecord(GDBResponse response);
		void handleNotifyAsyncRecord(GDBResponse response);
		
		// Handles all types of stream records
		void handleStreamRecords(GDBResponse response);
		
		
		
		
		// Response queue handling stuff
		
		thread m_dispatchThread;
		mutex m_responseQueueMutex;
		deque<GDBResponse> m_responseQueue;
		
		static void handlerDispatchThreadThunk(GDBMI *obj) { obj->handlerDispatchThread(); }
		void handlerDispatchThread();
		
		
		
		
		// Callback handling stuff
		
		void registerCallback(string token, CmdCallback cb);
		CallbackIter findCallback(string token);
		void eraseCallback(std::map<string, GDBMI::CmdCallback>::iterator &iter);
		mutex m_pendingCmdMutex;
		
		// This map is used to register callbacks when we receive a response
		// to a command we sent. We use the token in the response to index
		// into this map and find the correct callback.
		std::map<string, CmdCallback> m_pendingCommands; // map<token, callback>
		
		
		
		// Output handling stuff
		void sendCommand(string cmd);
		mutex m_sendCmdMutex;
		
		uint32_t getToken()
		{
			m_randMutex.lock();
			if(m_randOffset >= (2048 - 3))
			{
				fillRandPool();
				m_randOffset = 0;
			}
			
			uint32_t ret = 0;
			memcpy(&ret, (m_randPool + m_randOffset), 3);
			m_randOffset += 3;
			m_randMutex.unlock();
			
			ret >>= 8;
			return ret;
		}
		
		bool fillRandPool()
		{
			FILE *fp = fopen("/def/urandom", "rb");
			
			if(fp != 0)
			{
				fread(m_randPool, 2048, 0, fp);
				fclose(fp);
				
				return true;
			}
			
			return false;
		}
		
		uint8_t m_randPool[2048];
		uint32_t m_randOffset;
		mutex m_randMutex;
		
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif
