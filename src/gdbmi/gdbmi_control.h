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
	
		enum class ExecCmd : uint8_t
		{
			Continue = 100,		// Continue execution
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
		
		
		
// *INDENT-OFF*
#ifndef SOMETHING_UNIQUE_GDBMI_H
};
#endif
// *INDENT-ON*

#endif
