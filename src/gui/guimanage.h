#ifndef UNIQUE_GUIMANAGE_H
#define UNIQUE_GUIMANAGE_H


#include "gui_defs.h"
#include "gui_child.h"
#include "gui_codeview.h"
#include "gui_tabpanel.h"
#include "gui_toolbar.h"
#include "gui_console.h"

#include "gdbmi/gdbmi.h"

using namespace ImGui;

#define FLAG_FUNC_CACHE_STALE			(((uint64_t) 1) << 0)
#define FLAG_GLOBVAR_CACHE_STALE		(((uint64_t) 1) << 1)
#define FLAG_DISASM_CACHE_STALE			(((uint64_t) 1) << 2)
#define FLAG_REGISTER_CACHE_STALE		(((uint64_t) 1) << 3)
#define FLAG_STACKTRACE_CACHE_STALE		(((uint64_t) 1) << 4)
#define FLAG_BREAKPOINT_CACHE_STALE		(((uint64_t) 1) << 5)


class GuiManager : public GuiParentWrapper
{
	public:
	
		GuiManager(SDL_Window *sdlWin, SDL_GLContext *glCtx);
		~GuiManager();
		
		// Runs the app, calls mainLoop()
		void run();
		void exit() { m_exitProgram = true; }
		
		
		void addChild(GuiChild *child) { child->setParent(this); m_guiChildren.push_back(child); }
		
		
		// Get fonts
		ImFont *getDefaultFont()		{ return m_defaultFont; }
		ImFont *getBoldFont()			{ return m_boldFont; }
		ImFont *getItalicFont()			{ return m_italicFont; }
		ImFont *getBoldItalicFont()		{ return m_boldItalicFont;  }
		
		
		// Access gui info
		ImVec2 getMainWindowSize() { return m_mainWindowSize; }
		
		// Get the color config value for a gui element type
		ImVec4 getColor(GuiItem item)
		{
			if(m_guiColors.find(item) == m_guiColors.end())
				return ImVec4(1.0, 0, 0, 1.0);
				
			return m_guiColors[item];
		}
		
		// Debug info caches
		mutex &getFuncListMutex() { return m_funcListMutex; }
		vector<GDBMI::SymbolObject> &getFuncList() { return m_funcSymbolCache; }
		
		mutex &getCodeLinesMtx() { return m_codeLinesMutex; }
		AsmDump &getCodeLines() { return m_codeLines; }
		
		mutex &getGlobVarListMtx() { return m_gvarListMutex; }
		vector<GDBMI::SymbolObject> &getGlobVarList() { return m_globalVarCache; }
		
		mutex &getRegListMutex() { return m_registerCacheMutex;}
		vector<GDBMI::RegisterInfo> &getRegList() { return m_registerCache; }
		
		mutex &getBacktraceMutex() { return m_backtraceMutex; }
		vector<GDBMI::FrameInfo> &getBacktrace() { return m_backtraceCache; }
		
		mutex &getBreakpointMutex() { return m_bpCacheMutex; }
		vector<GDBMI::BreakpointInfo> &getBreakpointList() { return m_breakpointCache; }
		
		
		static void updateNotifyCBThunk(GDBMI::UpdateType updateType, void *obj)
		{ ((GuiManager *) obj)->updateNotifyCB(updateType); }
		
		static void cacheHandlerThreadThunk(GuiManager *obj)
		{ obj->cacheHandlerThread(); }
		
	private:
	
		// Application main loop. Returns when the user exits the program
		void mainLoop();
		
		void renderFrame();
		
		// Handles keyboard/mouse input
		void handleInput();
		
		vector<GDBMI::SymbolObject> m_funcSymbolCache;
		mutex m_funcListMutex;
		
		vector<GDBMI::SymbolObject> m_globalVarCache;
		mutex m_gvarListMutex;
		
		AsmDump m_codeLines;
		mutex m_codeLinesMutex;
		
		vector<GDBMI::RegisterInfo> m_registerCache;
		mutex m_registerCacheMutex;
		
		vector<GDBMI::FrameInfo> m_backtraceCache;
		mutex m_backtraceMutex;
		
		vector<GDBMI::BreakpointInfo> m_breakpointCache;
		mutex m_bpCacheMutex;
		
		void updateNotifyCB(GDBMI::UpdateType updateType);
		
		// Handles updating the various caches when update notifications come in
		void cacheHandlerThread();
		
		thread m_cacheThread;
		mutex m_cacheFlagMutex;
		uint64_t m_cacheStaleFlags = 0;
		void setCacheFlag(uint64_t f) 		{ m_cacheStaleFlags |= f; }
		void clearCacheFlag(uint64_t f)		{ m_cacheStaleFlags &= ~f; }
		bool isFlagSet(uint64_t f)			{ return (m_cacheStaleFlags & f) != 0; }
		
		
		ImGuiIO *m_io = 0;
		ImFont *m_defaultFont = 0;
		ImFont *m_boldFont = 0;
		ImFont *m_italicFont = 0;
		ImFont *m_boldItalicFont = 0;
		
		vector<GuiChild *> m_guiChildren;
		ImVec2 m_mainWindowSize = {0.0f, 0.0f};
		
		bool m_exitProgram = false;
		
		map<GuiItem, ImVec4> m_guiColors;
		
		SDL_Window *m_sdlWindow;
		SDL_GLContext *m_glContext;
		ImGuiContext *m_imGuiCtx;
};


#endif
