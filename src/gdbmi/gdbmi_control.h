#ifndef UNIQUE_GDBMI_CONTROL_H
#define UNIQUE_GDBMI_CONTROL_H

#ifndef SOMETHING_UNIQUE_GDBMI_H
#include "gdbmi_private.h"

class GDBMI
{

#define SOMETHING_UNIQUE_GDBMI_H

#undef SOMETHING_UNIQUE_GDBMI_H

#endif

	public:
	
		// Called by the GDBMI constructor
		void initControl();
		
		// Called by the GDBMI destructor
		void destroyControl();
		
		enum class ExecCmd : uint8_t
		{
			Continue = 10,		// Continue execution
			Finish,				// Execute until current function returns
			Interrupt,			// Interrupt currently executing program
			Jump,				// Jump straight to address
			Next,				// Next source line, don't step into functions
			NextInstruction,	// Nextd instruction, don't step into calls
			Return,				// Immediately return from function without executing
			Run,				// Run the program from the beginning
			Step,				// Step source line, do step into functions
			StepInstruction,	// Step instruction, do step into calls
			Until 				// Run until line/address
		};
		
		void doExecCommand(ExecCmd cmd, string location = "");
		
		enum class FileCmd : uint8_t
		{
			FileExec = 30,			// Load inferior to execute without symbols
			FileExecWithSymbols,	// Load inferior to execute and read symbols from file
			FileListSharedLibs 		// List the shared libraries in the inferior
			
			// Unimplemented:
			// -file-list-exec-source-file Command
			// -file-list-exec-source-files Command
			// -file-symbol-file Command
		};
		
		void doFileCommand(FileCmd cmd, string arg);
		
		void attachToPID(string pid);
		void detachInferior();
		void setInferiorArgs(string args);
		
		void insertBreakpointAtAddress(string addr);
		void deleteBreakpoint(uint32_t bpNum);
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif
