#ifndef UNIQUE_GDBMI_DATA_H
#define UNIQUE_GDBMI_DATA_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#endif
	public:
	
		enum class UpdateType : uint64_t
		{
			FuncSymbols 	= (1 << 0),
			GVarSymbols 	= (1 << 1),
			Disassembly 	= (1 << 2),
			RegisterInfo	= (1 << 3),
			Backtrace		= (1 << 4)
		};
		
		typedef void (*NotifyCallback)(UpdateType updType, void *userData);
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
		
		struct RegisterInfo
		{
			string regName;
			string regValue;
			uint32_t regSize; // Size of register in bytes (rax = 8, eax = 4, etc)
			bool updated = false;
		};
		
		struct FrameVariable
		{
			string name;
			string type;
			string value;
			bool isArg = false;
		};
		
		struct FrameInfo
		{
			uint32_t level = 0;
			string addr;
			string func;
			string file;
			string fullname;
			uint32_t line = 0;
			string arch;
			
			vector<FrameVariable> vars;
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
		
		NotifyCallback m_notifyCallback = 0;
		void *m_notifyUserData = 0;
		
		mutex m_regNameListMutex;
		vector<string> m_regNameList;
		
		mutex m_regValListMutex;
		vector<RegisterInfo> m_regValList;
		
		mutex m_backtraceMutex;
		vector<FrameInfo> m_backtrace;
		
		void requestFunctionSymbols();
		void requestGlobalVarSymbols();
		
		void requestCurrentExecPos();
		
	public:
	
		void setNotifyCallback(NotifyCallback cbFunc, void *userData = 0)
		{ m_notifyCallback = cbFunc; m_notifyUserData = userData; }
		
		void requestDisassembleAddr(string addr);
		void requestDisassembleFunc(string func) { requestDisassembleAddr(func); }
		void requestDisassembleLine(string file, string line);
		
		vector<SymbolObject> getFunctionSymbols();
		vector<SymbolObject> getGlobalVarSymbols();
		vector<RegisterInfo> getRegisters();
		vector<FrameInfo> getBacktrace();
		
		CurrentInstruction getCurrentExecutionPos();
		vector<DisassemblyInstruction> getDisassembly();
		
		StepFrame getStepFrame();
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif


