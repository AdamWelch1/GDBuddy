#include "gdbmi_private.h"
#include "gdbmi.h"


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
