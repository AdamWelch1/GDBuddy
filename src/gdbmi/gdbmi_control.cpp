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
			sendCommand("-exec-finish");
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
