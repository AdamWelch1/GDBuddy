#include "gdbmi_private.h"
#include "gdbmi.h"


void GDBMI::initControl()
{

}

void GDBMI::destroyControl()
{

}


void GDBMI::doExecCommand(ExecCmd cmd, string location)
{
	switch(cmd)
	{
		case ExecCmd::Continue:
		{
			sendCommand("-exec-continue");
		}
		break;
		
		case ExecCmd::Finish:
		{
			string cmdToken = getTokenStr();
			auto finishCB = [&](GDBMI * obj, GDBResponse resp) -> void
			{
				requestCurrentExecPos();
				requestDisassembleAddr("$pc");
			};
			
			registerCallback(cmdToken, finishCB);
			sendCommand(cmdToken + "-exec-finish");
		}
		break;
		
		case ExecCmd::Interrupt:
		{
			sendCommand("-exec-interrupt");
		}
		break;
		
		case ExecCmd::Jump:
		{
			sendCommand("-exec-jump " + location);
		}
		break;
		
		case ExecCmd::Next:
		{
			sendCommand("-exec-next");
		}
		break;
		
		case ExecCmd::NextInstruction:
		{
			sendCommand("-exec-next-instruction");
		}
		break;
		
		case ExecCmd::Return:
		{
			sendCommand("-exec-return");
		}
		break;
		
		case ExecCmd::Run:
		{
			sendCommand("-exec-run --start");
			// sendCommand("-exec-run");
		}
		break;
		
		case ExecCmd::Step:
		{
			sendCommand("-exec-step");
		}
		break;
		
		case ExecCmd::StepInstruction:
		{
			sendCommand("-exec-step-instruction");
		}
		break;
		
		case ExecCmd::Until:
		{
			sendCommand("-exec-until " + location);
		}
		break;
	}
}

void GDBMI::doFileCommand(FileCmd cmd, string arg)
{
	switch(cmd)
	{
		case FileCmd::FileExec:
		{
			logPrintf(LogLevel::Error, "\n-file-exec-file command not implemented yet\n");
		}
		break;
		
		case FileCmd::FileExecWithSymbols:
		{
			string token = getTokenStr();
			auto inferiorLoadCB = [&](GDBMI * obj, GDBResponse resp) -> void
			{
				setState(GDBState::Stopped, "Inferior loaded, not running");
				obj->requestFunctionSymbols();
				obj->requestGlobalVarSymbols();
				
				
				string tok2 = obj->getTokenStr();
				registerCallback(tok2, GDBMI::getregNamesCallbackThunk);
				
				sendCommand(tok2 + "-data-list-register-names");
				
				CallbackIter cb;
				if(findCallback(resp.recordToken, cb) == true)
					eraseCallback(cb);
			};
			
			registerCallback(token, inferiorLoadCB);
			sendCommand(token + string("-file-exec-and-symbols ") + arg);
		}
		break;
		
		case FileCmd::FileListSharedLibs:
		{
			logPrintf(LogLevel::Error, "-file-symbol-file command not implemented yet\n");
		}
		break;
		
	}
}

void GDBMI::attachToPID(string pid)
{
	string token = getTokenStr();
	auto inferiorLoadCB = [&](GDBMI * obj, GDBResponse resp) -> void
	{
		setState(GDBState::Stopped, "Inferior loaded, not running");
		obj->requestFunctionSymbols();
		obj->requestGlobalVarSymbols();
		
		
		string tok2 = obj->getTokenStr();
		registerCallback(tok2, GDBMI::getregNamesCallbackThunk);
		
		sendCommand(tok2 + "-data-list-register-names");
		
		CallbackIter cb;
		if(findCallback(resp.recordToken, cb) == true)
			eraseCallback(cb);
	};
	
	registerCallback(token, inferiorLoadCB);
	sendCommand(token + string("-target-attach ") + pid);
}

void GDBMI::detachInferior()
{
	string token = getTokenStr();
	auto detachCB = [](GDBMI * obj, GDBResponse resp) -> void
	{
		obj->refreshData();
	};
	
	registerCallback(token, detachCB);
	sendCommand("-target-detach");
}


void GDBMI::insertBreakpointAtAddress(string addr)
{

	auto insertCB = [&](GDBMI * obj, GDBResponse resp) -> void
	{
		obj->requestBreakpointList();
	};
	
	string cmdToken = getTokenStr();
	registerCallback(cmdToken, insertCB);
	sendCommand(cmdToken + string("-break-insert *") + addr);
}

void GDBMI::deleteBreakpoint(uint32_t bpNum)
{
	auto deleteCB = [&](GDBMI * obj, GDBResponse resp) -> void
	{
		obj->requestBreakpointList();
	};
	
	string cmdToken = getTokenStr();
	registerCallback(cmdToken, deleteCB);
	sendCommand(cmdToken + string("-break-delete ") + std::to_string(bpNum));
}
