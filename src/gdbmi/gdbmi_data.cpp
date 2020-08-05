#include "gdbmi_private.h"
#include "gdbmi.h"

#define printf(a, ...) logPrintf(LogLevel::NeedsFix, a,## __VA_ARGS__)

void GDBMI::requestFunctionSymbols()
{
	string token = getTokenStr();
	registerCallback(token, getFuncSymbolsCallbackThunk);
	sendCommand(token + "-symbol-info-functions");
}

void GDBMI::requestGlobalVarSymbols()
{
	string token = getTokenStr();
	registerCallback(token, getGlobalVarSymbolsCallbackThunk);
	sendCommand(token + "-symbol-info-variables");
}

void GDBMI::requestCurrentExecPos()
{
	auto updateCurrentPosCB = [](GDBMI * obj, GDBResponse r)
	{
		string data = r.recordData;
		KVPair kvp = obj->parserGetKVPair(data);
		
		string rawPos = kvp.second;
		
		if(rawPos.front() == '"')
			rawPos.erase(rawPos.begin());
			
		if(rawPos.back() == '"')
			rawPos.pop_back();
			
		size_t pos = rawPos.find_first_of(' ');
		
		if(pos != string::npos)
		{
			string addr = rawPos.substr(0, pos);
			string name = rawPos.substr(pos + 1);
			uint64_t addrInt = strtoull(addr.c_str(), 0, 16);
			
			if(name.front() == '<')
				name.erase(name.begin());
				
			if(name.back() == '>')
				name.pop_back();
				
			obj->m_curExecPosMutex.lock();
			obj->m_currentExecPos = {addrInt, name};
			obj->m_curExecPosMutex.unlock();
		}
	};
	
	string token = getTokenStr();
	registerCallback(token, updateCurrentPosCB);
	sendCommand(token + "-data-evaluate-expression $pc");
}

void GDBMI::requestDisassembleAddr(string addr)
{
	string token = getTokenStr();
	registerCallback(token, getDisassemblyCallbackThunk);
	sendCommand(token + string("-data-disassemble -a ") + addr + " 0");
}

void GDBMI::requestDisassembleLine(string file, string line)
{
	string token = getTokenStr();
	registerCallback(token, getDisassemblyCallbackThunk);
	sendCommand(token + string("-data-disassemble -f ") + file + string(" -l ") + line + " 0");
}

void GDBMI::requestBreakpointList()
{
	string cmdToken = getTokenStr();
	registerCallback(cmdToken, GDBMI::bpListCallbackThunk);
	sendCommand(cmdToken + "-break-list");
}


vector<GDBMI::SymbolObject> GDBMI::getFunctionSymbols()
{
	vector<SymbolObject> ret;
	m_funcSymMutex.lock();
	ret = m_functionSymbols;
	m_funcSymMutex.unlock();
	
	return ret;
}

vector<GDBMI::SymbolObject> GDBMI::getGlobalVarSymbols()
{
	vector<SymbolObject> ret;
	m_globalVarMutex.lock();
	ret = m_globalVarSymbols;
	m_globalVarMutex.unlock();
	
	return ret;
}

vector<GDBMI::RegisterInfo> GDBMI::getRegisters()
{
	vector<RegisterInfo> ret;
	m_regValListMutex.lock();
	ret = m_regValList;
	m_regValListMutex.unlock();
	
	return ret;
}

vector<GDBMI::FrameInfo> GDBMI::getBacktrace()
{
	vector<FrameInfo> ret;
	m_backtraceMutex.lock();
	ret = m_backtrace;
	m_backtraceMutex.unlock();
	
	return ret;
}

vector<GDBMI::BreakpointInfo> GDBMI::getBpList()
{
	vector<BreakpointInfo> ret;
	m_breakPointMutex.lock();
	ret = m_breakPointList;
	m_breakPointMutex.unlock();
	
	return ret;
}

GDBMI::CurrentInstruction GDBMI::getCurrentExecutionPos()
{
	CurrentInstruction ret = {0, ""};
	m_curExecPosMutex.lock();
	ret = m_currentExecPos;
	m_curExecPosMutex.unlock();
	
	return ret;
}

vector<GDBMI::DisassemblyInstruction> GDBMI::getDisassembly()
{
	vector<DisassemblyInstruction> ret;
	m_disasLinesMutex.lock();
	ret = m_disasLines;
	m_disasLinesMutex.unlock();
	
	return ret;
}

GDBMI::StepFrame GDBMI::getStepFrame()
{
	StepFrame ret;
	m_stepFrameMutex.lock();
	ret = m_stepFrame;
	m_stepFrameMutex.unlock();
	
	return ret;
}
