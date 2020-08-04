#include "gdbmi_private.h"
#include "gdbmi.h"

#define printf(a, ...) logPrintf(LogLevel::NeedsFix, a,## __VA_ARGS__)

void GDBMI::runningCallback(GDBResponse resp)
{
	setState(GDBState::Running, "Inferior is running");
	logPrintf(LogLevel::Info, "Inferior is running\n");
}

void GDBMI::stoppedCallback(GDBResponse resp)
{
	m_stepFrameMutex.lock();
	m_stepFrame.reset();
	m_stepFrameMutex.unlock();
	
	setState(GDBState::Stopped, "Inferior has stopped");
	logPrintf(LogLevel::Info, "Inferior stopped\n");
	
	if(resp.recordClass == "error")
		logPrintf(LogLevel::Debug, "stoppedCallback() error\n");
		
		
	sendCommand("-stack-list-variables 1");
	
	string regToken = getTokenStr();
	registerCallback(regToken, GDBMI::getregValsCallbackThunk);
	sendCommand(regToken + "-data-list-register-values x");
	
	string respData = resp.recordData;
	KVPair rootPair = parserGetKVPair(respData);
	
	if(rootPair.first == "reason")
	{
		if(rootPair.second == "breakpoint-hit")
		{
			setState(GDBState::Stopped, "Inferior stopped: breakpoint hit");
			
			requestCurrentExecPos();
			requestDisassembleAddr("$pc");
			
			CallbackIter cbIter;
			if(findCallback(rootPair.second, cbIter) == true)
			{
				cbIter->second(this, resp);
				return;
			}
		}
		
		if(rootPair.second == "exited-normally")
		{
			setState(GDBState::Exited, "Inferior exited: Exited normally");
			//
			return;
		}
		
		if(rootPair.second == "exited-signalled")
		{
			KVPair sigName = parserGetKVPair(respData);
			setState(GDBState::Exited, string("Inferior exited: Received '") + sigName.second + "' signal");
			
			return;
		}
		
		if(rootPair.second == "signal-received")
		{
			requestCurrentExecPos();
			requestDisassembleAddr("$pc");
			
			KVPair sigName = parserGetKVPair(respData);
			setState(GDBState::Stopped, string("Inferior stopped: Received '") + sigName.second + "' signal");
			
			return;
		}
		
		if(rootPair.second == "end-stepping-range")
		{
			setState(GDBState::Stopped, "Inferior stopped: Finished stepping");
			endStepCallback(resp);
			
			m_stepFrameMutex.lock();
			if(m_stepFrame.isValid)
				requestDisassembleAddr(m_stepFrame.address);
			else
				requestDisassembleAddr("$pc");
			m_stepFrameMutex.unlock();
			
			return;
		}
		
		// Hopefully, this is a catch-all for all non-exit stop events
		if(rootPair.second.find("exited-") == string::npos)
		{
			requestCurrentExecPos();
			requestDisassembleAddr("$pc");
			//
		}
		
		// Hopefully, this is a catch-all for all exit events
		if(rootPair.second.find("exited-") == string::npos)
		{
			setState(GDBState::Exited, "Inferior exited: Reason unknown");
			//
		}
		
		
		
		logPrintf(LogLevel::Debug, "Stop data: %s\n", resp.recordData.c_str());
	}
}

void GDBMI::endStepCallback(GDBResponse resp)
{
	StepFrame newStepFrame;
	
	string recordData = resp.recordData;
	KVPair framePair = parserGetKVPair(recordData); // Discard this one
	
	framePair = parserGetKVPair(recordData);
	string frameData = framePair.second;
	
	
	// frame={addr="0x000055555555fa90",func="??",args=[],arch="i386:x86-64"},
	// thread-id="1",
	// stopped-threads="all",
	// core="6"
	
	if(framePair.first != "frame")
	{
		m_stepFrameMutex.lock();
		m_stepFrame.reset();
		m_stepFrameMutex.unlock();
		
		return;
	}
	
	string frameTuple = parserGetTuple(frameData);
	KVPairVector frameKVP;
	parserGetKVPairs(frameTuple, frameKVP);
	
	for(auto &kvp : frameKVP)
	{
		if(kvp.first == "addr")
			newStepFrame.address = kvp.second;
		else if(kvp.first == "func")
			newStepFrame.func = kvp.second;
		else if(kvp.first == "args")
			newStepFrame.args = kvp.second;
	}
	
	KVPair tidPair = parserGetKVPair(recordData);
	
	if(tidPair.first == "thread-id")
		newStepFrame.threadID = tidPair.second;
		
	if(newStepFrame.address.length() > 0 &&
			newStepFrame.func.length() > 0 &&
			newStepFrame.args.length() > 0 &&
			newStepFrame.threadID.length() > 0)
		newStepFrame.isValid = true;
	else
		newStepFrame.isValid = false;
		
	// printf("New frame is valid: %u\n", newStepFrame.isValid);
	m_stepFrameMutex.lock();
	m_stepFrame = newStepFrame;
	m_stepFrameMutex.unlock();
	
	m_curExecPosMutex.lock();
	m_currentExecPos.first = stoull(newStepFrame.address, 0, 16);
	m_currentExecPos.second = newStepFrame.func;
	m_curExecPosMutex.unlock();
}

void GDBMI::bpCreatedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Verbose, "Breakpoint created\n");
}

void GDBMI::bpDeletedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Verbose, "Breakpoint deleted\n");
}

void GDBMI::bpModifiedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Verbose, "Breakpoint modified\n");
}

void GDBMI::bpHitCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Info, "Breakpoint hit\n");
}



void GDBMI::libLoadedCallback(GDBResponse resp)
{
	// printf("Library loaded\n");
}

void GDBMI::libUnloadedCallback(GDBResponse resp)
{
	// printf("Library unloaded\n");
}



void GDBMI::threadCreatedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Info, "Thread created\n");
}

void GDBMI::threadSelectedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Info, "Thread selected\n");
}

void GDBMI::threadExitedCallback(GDBResponse resp)
{
	logPrintf(LogLevel::Info, "Thread exited\n");
}



void GDBMI::getFuncSymbolsCallback(GDBResponse resp)
{
	if(resp.recordData.length() > 0)
	{
		string rawData = resp.recordData;
		KVPair rootPair = parserGetKVPair(rawData);
		
		if(rootPair.first != "symbols" || getItemType(rootPair.second[0]) != ParseItemType::Tuple)
			return;
			
		string symbolTuple = parserGetTuple(rootPair.second);
		KVPair symKVP = parserGetKVPair(symbolTuple);
		
		if(symKVP.first != "debug" || getItemType(symKVP.second[0]) != ParseItemType::List)
			return;
			
		string symbolList = symKVP.second;
		
		// Strip '[' and ']' off the list
		symbolList.erase(symbolList.begin());
		symbolList.erase(symbolList.end() - 1);
		
		ListItemVector symFiles;
		parserGetListItems(symbolList, symFiles);
		
		for(auto &file : symFiles)
		{
			string fileTuple = parserGetTuple(file);
			KVPairVector sfVector;
			parserGetKVPairs(fileTuple, sfVector);
			
			string fullPath;
			string shortName;
			
			for(auto &fileItem : sfVector)
			{
				if(fileItem.first == "filename")
				{
					// printf("Filename: %s\n", fileItem.second.c_str());
					shortName = fileItem.second;
					continue;
				}
				
				if(fileItem.first == "fullname")
				{
					// printf("Full name: %s\n", fileItem.second.c_str());
					fullPath = fileItem.second;
					continue;
				}
				
				if(fileItem.first == "symbols")
				{
					string symList = fileItem.second;
					symList.erase(symList.begin());
					symList.erase(symList.end() - 1);
					
					ListItemVector symTuples;
					parserGetListItems(symList, symTuples);
					
					for(auto &symTup : symTuples)
					{
						SymbolObject tmp;
						tmp.fullPath = fullPath;
						tmp.shortName = shortName;
						tmp.isActive = false;
						
						string symbolInfo = parserGetTuple(symTup);
						KVPairVector symParts;
						parserGetKVPairs(symbolInfo, symParts);
						
						for(auto &sym : symParts)
						{
							if(sym.first == "line")
								tmp.lineNumber = sym.second;
							else if(sym.first == "name")
								tmp.name = sym.second;
							else if(sym.first == "type")
								tmp.type = sym.second;
							else if(sym.first == "description")
								tmp.description = sym.second;
								
							// printf("\t%s: %s\n", sym.first.c_str(), sym.second.c_str());
						}
						
						m_funcSymMutex.lock();
						m_functionSymbols.push_back(tmp);
						m_funcSymMutex.unlock();
					}
				}
			}
		}
	}
	
	CallbackIter cb;
	if(findCallback(resp.recordToken, cb) == true)
		eraseCallback(cb);
		
	if(m_notifyCallback != 0)
		m_notifyCallback(UpdateType::FuncSymbols, m_notifyUserData);
}

void GDBMI::getGlobalVarSymbolsCallback(GDBResponse resp)
{
	if(resp.recordData.length() > 0)
	{
		string rawData = resp.recordData;
		KVPair rootPair = parserGetKVPair(rawData);
		
		if(rootPair.first != "symbols" || getItemType(rootPair.second[0]) != ParseItemType::Tuple)
			return;
			
		string symbolTuple = parserGetTuple(rootPair.second);
		KVPair symKVP = parserGetKVPair(symbolTuple);
		
		if(symKVP.first != "debug" || getItemType(symKVP.second[0]) != ParseItemType::List)
			return;
			
		string symbolList = symKVP.second;
		
		// Strip '[' and ']' off the list
		symbolList.erase(symbolList.begin());
		symbolList.erase(symbolList.end() - 1);
		
		ListItemVector symFiles;
		parserGetListItems(symbolList, symFiles);
		
		for(auto &file : symFiles)
		{
			string fileTuple = parserGetTuple(file);
			KVPairVector sfVector;
			parserGetKVPairs(fileTuple, sfVector);
			
			string fullPath;
			string shortName;
			
			for(auto &fileItem : sfVector)
			{
				if(fileItem.first == "filename")
				{
					// printf("Filename: %s\n", fileItem.second.c_str());
					shortName = fileItem.second;
					continue;
				}
				
				if(fileItem.first == "fullname")
				{
					// printf("Full name: %s\n", fileItem.second.c_str());
					fullPath = fileItem.second;
					continue;
				}
				
				if(fileItem.first == "symbols")
				{
					string symList = fileItem.second;
					symList.erase(symList.begin());
					symList.erase(symList.end() - 1);
					
					ListItemVector symTuples;
					parserGetListItems(symList, symTuples);
					
					for(auto &symTup : symTuples)
					{
						SymbolObject tmp;
						tmp.fullPath = fullPath;
						tmp.shortName = shortName;
						tmp.isActive = false;
						
						string symbolInfo = parserGetTuple(symTup);
						KVPairVector symParts;
						parserGetKVPairs(symbolInfo, symParts);
						
						for(auto &sym : symParts)
						{
							if(sym.first == "line")
								tmp.lineNumber = sym.second;
							else if(sym.first == "name")
								tmp.name = sym.second;
							else if(sym.first == "type")
								tmp.type = sym.second;
							else if(sym.first == "description")
								tmp.description = sym.second;
								
							// printf("\t%s: %s\n", sym.first.c_str(), sym.second.c_str());
						}
						
						m_globalVarMutex.lock();
						m_globalVarSymbols.push_back(tmp);
						m_globalVarMutex.unlock();
					}
				}
			}
		}
	}
	
	CallbackIter cb;
	if(findCallback(resp.recordToken, cb) == true)
		eraseCallback(cb);
		
	if(m_notifyCallback != 0)
		m_notifyCallback(UpdateType::GVarSymbols, m_notifyUserData);
}

void GDBMI::getDisassemblyCallback(GDBResponse resp)
{
	vector<DisassemblyInstruction> tmpBuf;
	
	if(resp.recordData.length() > 0)
	{
		m_disasLinesMutex.lock();
		m_disasLines.clear();
		m_disasLinesMutex.unlock();
		
		string rawDisas = resp.recordData;
		KVPair rootPair = parserGetKVPair(rawDisas);
		string rawList = rootPair.second;
		
		if(rawList.front() == '[')
			rawList.erase(rawList.begin());
			
		if(rawList.back() == ']')
			rawList.pop_back();
			
		ListItemVector disasList;
		parserGetListItems(rawList, disasList);
		
		for(auto &asmTuple : disasList)
		{
			string tuple = parserGetTuple(asmTuple);
			
			KVPairVector kvpList;
			parserGetKVPairs(tuple, kvpList);
			
			DisassemblyInstruction inst;
			
			for(auto &kvp : kvpList)
			{
				if(kvp.first == "address")
				{
					inst.addrStr = kvp.second;
					inst.address = strtoull(kvp.second.c_str(), 0, 16);
				}
				else if(kvp.first == "func-name")
					inst.funcName = kvp.second;
				else if(kvp.first == "offset")
					inst.offset = kvp.second;
				else if(kvp.first == "inst")
					inst.instruction = kvp.second;
			}
			
			tmpBuf.push_back(inst);
		}
		
		m_disasLinesMutex.lock();
		m_disasLines.swap(tmpBuf);
		m_disasLinesMutex.unlock();
	}
	
	CallbackIter cb;
	if(findCallback(resp.recordToken, cb) == true)
		eraseCallback(cb);
		
	if(m_notifyCallback != 0)
		m_notifyCallback(UpdateType::Disassembly, m_notifyUserData);
}

void GDBMI::getregNamesCallback(GDBResponse resp)
{
	if(resp.recordData.length() > 0)
	{
		string regNames = resp.recordData;
		KVPair rootKVP = parserGetKVPair(regNames);
		
		if(rootKVP.first == "register-names")
		{
			string nameList = rootKVP.second;
			if(nameList[0] == '[')
				nameList.erase(nameList.begin());
				
			if(*(nameList.end() - 1) == ']')
				nameList.pop_back();
				
			m_regNameListMutex.lock();
			m_regNameList.clear();
			
			while(nameList.length() > 1)
			{
				string item = parserGetItem(nameList);
				m_regNameList.push_back(item);
			}
			m_regNameListMutex.unlock();
		}
	}
	
	CallbackIter cb;
	if(findCallback(resp.recordToken, cb) == true)
		eraseCallback(cb);
}

void GDBMI::getregValsCallback(GDBResponse resp)
{
	if(resp.recordData.length() > 0)
	{
		string regData = resp.recordData;
		KVPair rootKVP = parserGetKVPair(regData);
		
		if(rootKVP.first == "register-values")
		{
			string regValList = rootKVP.second;
			
			if(regValList[0] == '[')
				regValList.erase(regValList.begin());
				
			if(regValList.back() == ']')
				regValList.pop_back();
				
			m_regNameListMutex.lock();
			m_regValListMutex.lock();
			
			auto oldRegList = m_regValList;
			m_regValList.clear();
			
			while(regValList.length() > 1)
			{
				string rvTuple = parserGetTuple(regValList);
				
				KVPair num = parserGetKVPair(rvTuple);
				KVPair val = parserGetKVPair(rvTuple);
				
				uint32_t regNum = strtoul(num.second.c_str(), 0, 10);
				string regVal = val.second;
				
				if(regNum >= m_regNameList.size())
					continue;
					
				if(m_regNameList[regNum].length() == 0)
					continue;
					
				RegisterInfo tmp;
				tmp.regName = m_regNameList[regNum];
				tmp.regSize = 0;
				tmp.regValue = regVal;
				
				if(regVal.length() > 1 && regVal[0] == '0' && regVal[1] == 'x') // If we have a hex number...
				{
					// Count the number of digits in the hex value
					
					uint32_t digitCount = 0;
					for(uint32_t i = 2; i < regVal.length(); i++)
					{
						if(regVal[i] >= '0' && regVal[i] <= '9')
							digitCount++;
						else
							break;
					}
					
					if(digitCount % 2 != 0)
						digitCount++;
						
					tmp.regSize = digitCount / 2; // Register size in bytes is (# of hex digits) / 2
				}
				
				// Check if this register value has changed
				tmp.updated = false;
				for(auto &oldreg : oldRegList)
				{
					if(oldreg.regName == tmp.regName)
					{
						if(oldreg.regValue != tmp.regValue)
							tmp.updated = true;
							
						break;
					}
				}
				
				// TODO: I still need to handle the SIMD registers here.
				//
				// I'm not sure if GDB returns values in a different format
				// depending on the processor architecture, so I'll need to
				// do some research.
				//
				// As an example, the XMM registers on an Intel i7 x64 processor
				// are returned by GDB in the following format:
				//
				//	{number="40",value="{v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0},
				//		v16_int8 = {0x0 <repeats 16 times>}, v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0,
				//		0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0},
				//		uint128 = 0x0}"},
				//
				// I'm thinking it would probably be a good idea to call a per-architecture handler
				// after processing the registers that have simple numerical values.
				
				m_regValList.push_back(tmp);
			}
			m_regNameListMutex.unlock();
			m_regValListMutex.unlock();
		}
	}
	
	CallbackIter cb;
	if(findCallback(resp.recordToken, cb) == true)
		eraseCallback(cb);
		
	if(m_notifyCallback != 0)
		m_notifyCallback(UpdateType::RegisterInfo, m_notifyUserData);
}
