#include "gdbmi_private.h"

#include "gdbmi.h"
#ifndef SOMETHING_UNIQUE_GDBMI_H
// #include "gdbmi_handlers.h"
#endif

void GDBMI::initHandlers()
{
	m_randOffset = 0;
	fillRandPool();
	
	m_dispatchThread = thread(GDBMI::handlerDispatchThreadThunk, this);
}

void GDBMI::destroyHandlers()
{
	m_dispatchThread.join();
}

void GDBMI::sendCommand(string cmd)
{
	if(*(cmd.end() - 1) != '\n')
		cmd += "\n";
		
	m_sendCmdMutex.lock();
	writePipe(cmd);
	m_sendCmdMutex.unlock();
}

void GDBMI::handleResponse(string &responseStr)
{
	static bool allowGDBConsole = false;
	if(responseStr.length() == 0)
		return;
		
	int32_t tokenEnd = -1;
	for(int32_t i = 0; i < (int32_t) responseStr.length(); i++)
	{
		if(responseStr[i] >= '0' && responseStr[i] <= '9')
			continue;
			
		tokenEnd = i;
		break;
	}
	
	GDBResponse resp;
	
	if(tokenEnd > 0)
	{
		resp.recordToken = responseStr.substr(0, tokenEnd);
		responseStr = responseStr.substr(tokenEnd);
	}
	
	char recordTypeIndicator = responseStr[0];
	resp.recordType = GDBRecordType::INVALID;
	switch(recordTypeIndicator)
	{
		// *INDENT-OFF*
		case '^': resp.recordType = GDBRecordType::Result; 			break;
		case '*': resp.recordType = GDBRecordType::ExecAsync; 		break;
		case '+': resp.recordType = GDBRecordType::StatusAsync; 	break;
		case '=': resp.recordType = GDBRecordType::NotifyAsync; 	break;
		case '~': resp.recordType = GDBRecordType::ConsoleStream;	break;
		case '@': resp.recordType = GDBRecordType::TargetStream;	break;
		case '&': resp.recordType = GDBRecordType::LogStream; 		break;
		// *INDENT-ON*
	}
	
	if(resp.recordType == GDBRecordType::INVALID)
		return;
		
	responseStr = responseStr.substr(1);
	
	if(resp.recordType == GDBRecordType::ConsoleStream ||
			resp.recordType == GDBRecordType::TargetStream ||
			resp.recordType == GDBRecordType::LogStream)
	{
		resp.recordData = responseStr;
		//
	}
	else
	{
		int32_t comma = -1;
		for(int32_t i = 0; i < responseStr.length(); i++)
		{
			if(responseStr[i] == ' ' || responseStr[i] == ',')
			{
				comma = i;
				break;
			}
		}
		if(comma == 0)
			return;
			
		if(comma > 0)
		{
			resp.recordClass = responseStr.substr(0, comma);
			resp.recordData = responseStr.substr(comma + 1);
		}
		else
			resp.recordClass = responseStr;
	}
	
		// *INDENT-OFF*
	{ 	// These two if's stop us from printing the gdb start-up text
		// Why? Because it's been Driving. Me. Crazy. Spamming the damn console >_<
		// *INDENT-ON*
		if(allowGDBConsole && resp.recordType == GDBRecordType::ConsoleStream)
		{
			if(resp.recordData.find("GNU gdb (GDB)") != string::npos)
			{
				allowGDBConsole = false;
				return;
			}
		}
		
		if(!allowGDBConsole && resp.recordType == GDBRecordType::ConsoleStream)
		{
			if(resp.recordData.find("apropos word") != string::npos)
				allowGDBConsole = true;
				
			return;
		}
	}
	
	if(resp.recordType != GDBRecordType::INVALID)
	{
		m_responseQueueMutex.lock();
		m_responseQueue.push_back(resp);
		m_responseQueueMutex.unlock();
	}
}

void GDBMI::handleResultRecord(GDBResponse response)
{
	if(response.recordClass == "error")
	{
		KVPair kvp = parserGetKVPair(response.recordData);
		printf("Error: %s\n", kvp.second.c_str());
	}
	
	printf("Unhandled Result record: Class: %s; Data: %s\n",
		   response.recordClass.c_str(), response.recordData.c_str());
}

void GDBMI::handleExecAsyncRecord(GDBResponse response)
{
	printf("Unhandled ExecAsync record: Class: %s; Data: %s\n",
		   response.recordClass.c_str(), response.recordData.c_str());
}

void GDBMI::handleStatusAsyncRecord(GDBResponse response)
{
	printf("Unhandled StatusAsync record: Class: %s; Data: %s\n",
		   response.recordClass.c_str(),
		   response.recordData.c_str());
}

void GDBMI::handleNotifyAsyncRecord(GDBResponse response)
{
	printf("Unhandled NotifyAsync record: Class: %s; Data: %s\n",
		   response.recordClass.c_str(),
		   response.recordData.c_str());
}

void GDBMI::handleStreamRecords(GDBResponse response)
{
	printf("Unhandled stream record: Data: %s\n", response.recordData.c_str());
}

void GDBMI::registerCallback(string token, CmdCallback cb)
{
	m_pendingCmdMutex.lock();
	m_pendingCommands[token] = cb;
	m_pendingCmdMutex.unlock();
}

GDBMI::CallbackIter GDBMI::findCallback(string token)
{
	m_pendingCmdMutex.lock();
	auto ret = m_pendingCommands.find(token);
	m_pendingCmdMutex.unlock();
	
	return ret;
}

void GDBMI::eraseCallback(std::map<string, GDBMI::CmdCallback>::iterator &iter)
{
	m_pendingCmdMutex.lock();
	m_pendingCommands.erase(iter);
	m_pendingCmdMutex.unlock();
}

void GDBMI::handlerDispatchThread()
{
	uint32_t threadCount = 0;
	deque<GDBResponse> responseList;
	mutex rlMutex;
	
	auto taskThread = [&]() -> void
	{
		rlMutex.lock();
		threadCount++;
		GDBResponse resp = responseList.front();
		responseList.pop_front();
		rlMutex.unlock();
		
		switch(resp.recordType)
		{
			// *INDENT-OFF*
			case GDBRecordType::Result:			handleResultRecord(resp);		break;
			case GDBRecordType::ExecAsync:		handleExecAsyncRecord(resp);	break;
			case GDBRecordType::StatusAsync:	handleStatusAsyncRecord(resp);	break;
			case GDBRecordType::NotifyAsync:	handleNotifyAsyncRecord(resp); 	break;
			case GDBRecordType::ConsoleStream:	handleStreamRecords(resp);		break;
			case GDBRecordType::TargetStream:	handleStreamRecords(resp);		break;
			case GDBRecordType::LogStream:		handleStreamRecords(resp);		break;
			case GDBRecordType::INVALID: 										break;
			// *INDENT-ON*
		}
		
		rlMutex.lock();
		threadCount--;
		rlMutex.unlock();
	};
	
	while(!m_exitThreads)
	{
		m_responseQueueMutex.lock();
		
		if(m_responseQueue.size() <= 0)
		{
			m_responseQueueMutex.unlock();
			usleep(1000 * 50);
			continue;
		}
		
		GDBResponse r = m_responseQueue.front();
		m_responseQueue.pop_front();
		m_responseQueueMutex.unlock();
		
		rlMutex.lock();
		while(threadCount >= GDB_HANDLER_THREAD_COUNT)
		{
			rlMutex.unlock();
			usleep(1000 * 10);
			rlMutex.lock();
		}
		
		responseList.push_back(r);
		rlMutex.unlock();
		
		thread task = thread(taskThread);
		task.detach();
	}
	
	// Wait for the worker threads to terminate
	rlMutex.lock();
	while(threadCount > 0)
	{
		rlMutex.unlock();
		usleep(1000 * 100);
		rlMutex.lock();
	}
	rlMutex.unlock();
}
