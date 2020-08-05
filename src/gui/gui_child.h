#ifndef UNIQUE_GUI_CHILD_H
#define UNIQUE_GUI_CHILD_H

#include "../gdbmi/gdbmi.h"
#include "gui_defs.h"

class GuiParentWrapper;

class GuiChild
{
	public:
	
		virtual void draw() = 0;
		virtual void setParent(void *p) { m_parent = (GuiParentWrapper *) p; }
		virtual void setSameLine(bool sl) { m_sameLine = sl; }
		virtual bool isSameLine() { return m_sameLine; }
		
	protected:
	
		GuiParentWrapper *m_parent = 0;
		bool m_sameLine = false;
};

// This class only exists to wrap the 'GuiManager' object
// so that objects derivced from 'GuiChild' can make calls
// into the parent gui object.
class GuiParentWrapper
{
	public:
	
		virtual ImVec4 getColor(GuiItem item) = 0;
		virtual void addChild(GuiChild *child) = 0;
		virtual ImFont *getDefaultFont() = 0;
		virtual ImFont *getBoldFont() = 0;
		virtual ImFont *getItalicFont() = 0;
		virtual ImFont *getBoldItalicFont() = 0;
		virtual ImVec2 getMainWindowSize() = 0;
		virtual mutex &getFuncListMutex() = 0;
		virtual vector<GDBMI::SymbolObject> &getFuncList() = 0;
		virtual mutex &getCodeLinesMtx() = 0;
		virtual AsmDump &getCodeLines() = 0;
		virtual mutex &getGlobVarListMtx() = 0;
		virtual vector<GDBMI::SymbolObject> &getGlobVarList() = 0;
		virtual mutex &getBreakpointMutex() = 0;
		virtual vector<GDBMI::BreakpointInfo> &getBreakpointList() = 0;
};

#endif
