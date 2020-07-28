#ifndef UNIQUE_GDBMI_DATA_H
#define UNIQUE_GDBMI_DATA_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#endif
	public:
	
		typedef pair<uint64_t, string> CurrentInstruction;
		
		struct SymbolObject
		{
			bool isActive;
			string fullPath;
			string shortName;
			string lineNumber;
			string name;
			string type;
			string description;
		};
		
		struct DisassemblyInstruction
		{
			uint64_t address;
			string addrStr;
			string funcName;
			string offset;
			string instruction;
		};
		
		struct StepFrame
		{
			// frame={addr="0x000055555555fa90",func="??",args=[],arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="6"
			bool isValid;
			string address;
			string func;
			string args;
			string threadID;
			
			void reset() { isValid = false; address = func = args = threadID = ""; }
			StepFrame() { reset(); }
		};
		
	private:
	
		vector<SymbolObject> m_functionSymbols;
		vector<SymbolObject> m_globalVarSymbols;
		mutex m_funcSymMutex;
		mutex m_globalVarMutex;
		
		// pair<$pc addr, func name>
		CurrentInstruction m_currentExecPos;
		mutex m_curExecPosMutex;
		
		vector<DisassemblyInstruction> m_disasLines;
		mutex m_disasLinesMutex;
		
		StepFrame m_stepFrame;
		mutex m_stepFrameMutex;
		
		
		void requestFunctionSymbols();
		void requestGlobalVarSymbols();
		
		void requestCurrentExecPos();
		
	public:
	
		void requestDisassembleAddr(string addr);
		void requestDisassembleFunc(string func) { requestDisassembleAddr(func); }
		void requestDisassembleLine(string file, string line);
		
		vector<SymbolObject> getFunctionSymbols();
		vector<SymbolObject> getGlobalVarSymbols();
		
		CurrentInstruction getCurrentExecutionPos();
		vector<DisassemblyInstruction> getDisassembly();
		
		StepFrame getStepFrame();
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


