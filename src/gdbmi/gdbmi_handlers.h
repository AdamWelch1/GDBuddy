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

		#ifdef BUILD_GDBMI_TESTS
	public:
		#else
	private:
		#endif
	
		struct GDBResponse;
		typedef std::function<void(GDBMI *, GDBResponse)> CmdCallback;
		typedef std::map<string, CmdCallback>::iterator CallbackIter;
		
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
		bool findCallback(string token, CallbackIter &out);
		void eraseCallback(CallbackIter &iter);
		mutex m_pendingCmdMutex;
		
		// This map is used to register callbacks when we receive a response
		// to a command we sent. We use the token in the response to index
		// into this map and find the correct callback.
		std::map<string, CmdCallback> m_pendingCommands; // map<token, callback>
		
		
		// Here we set up some default handlers for certain events.
		// For these callbacks, we register them under the record class name
		// that we want the callback to execute for. The "handle_*_Records()" functions
		// look for a callback under the class name if no token callback is found.
		
		// Note: **These callbacks are implemented in 'gdbmi_handlers_cb.cpp'**
		
		// ** execution state callbacks ** //
		
	public:
		static void runningCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->runningCallback(resp); }
		
	private:
		void runningCallback(GDBResponse resp);
		
	public:
		static void stoppedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->stoppedCallback(resp); }
		
	private:
		void stoppedCallback(GDBResponse resp);
		
	public:
		static void endStepCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->endStepCallback(resp); }
		
	private:
		void endStepCallback(GDBResponse resp);
		
		
		
		// ** breakpoint callbacks ** //
		
	public:
		static void bpCreatedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->bpCreatedCallback(resp); }
		
	private:
		void bpCreatedCallback(GDBResponse resp);
		
	public:
		static void bpDeletedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->bpDeletedCallback(resp); }
		
	private:
		void bpDeletedCallback(GDBResponse resp);
		
	public:
		static void bpModifiedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->bpModifiedCallback(resp); }
		
	private:
		void bpModifiedCallback(GDBResponse resp);
		
	public:
		static void bpHitCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->bpHitCallback(resp); }
		
	private:
		void bpHitCallback(GDBResponse resp);
		
		
		// ** shared library callbacks ** //
		
	public:
		static void libLoadedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->libLoadedCallback(resp); }
		
	private:
		void libLoadedCallback(GDBResponse resp);
		
	public:
		static void libUnloadedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->libUnloadedCallback(resp); }
		
	private:
		void libUnloadedCallback(GDBResponse resp);
		
		
		// ** thread event callbacks ** //
		
	public:
		static void threadCreatedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->threadCreatedCallback(resp); }
		
	private:
		void threadCreatedCallback(GDBResponse resp);
		
	public:
		static void threadSelectedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->threadSelectedCallback(resp); }
		
	private:
		void threadSelectedCallback(GDBResponse resp);
		
	public:
		static void threadExitedCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->threadExitedCallback(resp); }
		
	private:
		void threadExitedCallback(GDBResponse resp);
		
		
		// ** data query response callbacks ** ///
		
	public:
		static void getFuncSymbolsCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->getFuncSymbolsCallback(resp); }
		
	private:
		void getFuncSymbolsCallback(GDBResponse resp);
		
	public:
		static void getGlobalVarSymbolsCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->getGlobalVarSymbolsCallback(resp); }
		
	private:
		void getGlobalVarSymbolsCallback(GDBResponse resp);
		
	public:
		static void getDisassemblyCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->getDisassemblyCallback(resp); }
		
	private:
		void getDisassemblyCallback(GDBResponse resp);
		
	public:
		static void getregNamesCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->getregNamesCallback(resp); }
		
	private:
		void getregNamesCallback(GDBResponse resp);
		
	public:
		static void getregValsCallbackThunk(GDBMI *obj, GDBResponse resp)
		{ obj->getregValsCallback(resp); }
		
	private:
		void getregValsCallback(GDBResponse resp);
		
		
		
		
		
		// Output handling stuff
		
	private:
	
		void sendCommand(string cmd);
		mutex m_sendCmdMutex;
		
		
		uint32_t getToken()
		{
			m_randMutex.lock();
			uint32_t ret = 0;
			
			while(ret == 0)
			{
				if(m_randOffset >= (2048 - 3))
				{
					fillRandPool();
					m_randOffset = 0;
				}
				
				memcpy(&ret, (m_randPool + m_randOffset), 3);
				m_randOffset += 3;
			}
			
			m_randMutex.unlock();
			
			// ret >>= 8;
			return ret;
		}
		
		string getTokenStr()
		{
			uint32_t tk = getToken();
			char intStr[128] = {0};
			sprintf(intStr, "%u", tk);
			
			return string(intStr);
		}
		
		bool fillRandPool()
		{
			FILE *fp = fopen("/dev/urandom", "rb");
			
			if(fp != 0)
			{
				fread(m_randPool, 2048, 1, fp);
				fclose(fp);
				
				return true;
			}
			else fprintf(stderr, "fopen(/dev/urandom, rb) failed!\n");
			
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
