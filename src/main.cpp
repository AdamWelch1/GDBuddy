#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>

#include <unistd.h>
#include <fcntl.h>

// #include "gdb_io.h"
#include "gdbmi/gdbmi.h"
#include "gui/guimanage.h"

#include "imgui/imgui.h"

GDBMI *gdb = 0;
GuiManager *gui = 0;

void breakpointsTabPainter(string tabName, void *userData);
void symbolTabPainter(string tabName, void *userData);
void registerTabPainter(string tabName, void *userData);
void backtraceTabPainter(string tabname, void *userData);

void signalHandler(int param)
{
	switch(param)
	{
		case SIGINT:
		{
			fprintf(stderr, "Interrupt ^C caught; Exiting...\n");
			
			if(gui != 0)
				gui->exit();
		}
		break;
		
		case SIGPIPE:
		{
		
		}
		break;
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	
	gdb = new GDBMI;
	
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}
	
	SDL_Window *window = 0;
	SDL_GLContext gl_context;
	const char *glsl_version = "#version 130";
	
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		
		// Create window with graphics context
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		
		SDL_WindowFlags window_flags = (SDL_WindowFlags)
									   (SDL_WINDOW_OPENGL |
										SDL_WINDOW_ALLOW_HIGHDPI
										// SDL_WINDOW_MAXIMIZED
									   );
									   
		SDL_Rect displaySize;
		SDL_GetDisplayUsableBounds(0, &displaySize);
		displaySize.h -= 30;
		
		window = SDL_CreateWindow("DebugUI",
								  0,
								  0,
								  displaySize.w,
								  displaySize.h,
								  window_flags);
								  
		gl_context = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, gl_context);
		SDL_GL_SetSwapInterval(1); // Enable vsync
		
		// Initialize OpenGL loader
		bool err = glewInit() != GLEW_OK;
		
		if(err)
		{
			fprintf(stderr, "Failed to initialize OpenGL loader!\n");
			return 1;
		}
	}
	
	gui = new GuiManager(window, &gl_context);
	
	GuiTabPanel leftPanel("SymbolsPanel", 0.2f, 0.6f);
	leftPanel.addTab("Local Func", symbolTabPainter);
	leftPanel.addTab("Imports", symbolTabPainter);
	leftPanel.addTab("Global Vars", symbolTabPainter);
	leftPanel.setSameLine(false);
	
	GuiTabPanel rightPanel("InferiorInfoPanel", 0.2f, 0.6f);
	rightPanel.addTab("Registers", registerTabPainter);
	rightPanel.addTab("Threads", 0);
	rightPanel.setSameLine(true);
	
	GuiCodeView codeView(0.6f, 0.6f);
	codeView.setFonts(gui->getDefaultFont(), gui->getBoldFont(), gui->getItalicFont(), gui->getBoldItalicFont());
	codeView.setSameLine(true);
	
	GuiToolbar toolbar;
	toolbar.setStatusCB([&]() -> string { return gdb->getStatusMsg(); });
	
	// Set up toolbar buttons
	{
		using ExecCmd = GDBMI::ExecCmd;
		ButtonInfo button;
		button.text = "Run";
		button.tooltip = "(R) Run program from the beginning";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Run); };
		button.chkKeyCmd = []() -> bool
		{ bool ret = gui->isKeyPressed('r'); if(ret) gui->clearKeyPress('r'); return ret; };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Pause";
		button.tooltip = "(Ctrl+C) Interrupt program execution";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Interrupt); };
		button.chkKeyCmd = []() -> bool
		{
			bool ret = gui->isKeyPressed('c');
			ret = (ret && gui->isKeyPressed(KEY_CTRL));
			if(ret)
				gui->clearKeyPress('c');
			return ret;
		};
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Continue";
		button.tooltip = "(C) Continue program execution";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Continue); };
		button.chkKeyCmd = []() -> bool
		{ bool ret = gui->isKeyPressed('c'); if(ret) gui->clearKeyPress('c'); return ret; };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step line";
		button.tooltip = "(S) Step source line";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Step); };
		button.chkKeyCmd = []() -> bool
		{ bool ret = gui->isKeyPressed('s'); if(ret) gui->clearKeyPress('s'); return ret; };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step over line";
		button.tooltip = "(Shift+S) Step over source line";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Next); };
		button.chkKeyCmd = []() -> bool
		{
			bool ret = gui->isKeyPressed('s');
			ret = (ret && gui->isKeyPressed(KEY_SHIFT));
			if(ret)
				gui->clearKeyPress('s');
			return ret;
		};
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step inst";
		button.tooltip = "(I) Step instruction";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::StepInstruction); };
		button.chkKeyCmd = []() -> bool
		{ bool ret = gui->isKeyPressed('i'); if(ret) gui->clearKeyPress('i'); return ret; };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step over inst";
		button.tooltip = "(Shift+I) Step over instruction";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::NextInstruction); };
		button.chkKeyCmd = []() -> bool
		{
			bool ret = gui->isKeyPressed('i');
			ret = (ret && gui->isKeyPressed(KEY_SHIFT));
			if(ret)
				gui->clearKeyPress('i');
			return ret;
		};
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Finish exec";
		button.tooltip = "(F) Execute until current function returns";
		button.onClick 	 = [&]() { gdb->doExecCommand(ExecCmd::Finish); };
		button.chkKeyCmd = []() -> bool
		{ bool ret = gui->isKeyPressed('f'); if(ret) gui->clearKeyPress('f'); return ret; };
		toolbar.addButton(button), button.onClick = 0;
	}
	
	GuiTabPanel stackPanel("StackPanel", 0.35, 0.33);
	stackPanel.setSameLine(true);
	stackPanel.addTab("Backtrace", backtraceTabPainter);
	stackPanel.addTab("Breakpoints", breakpointsTabPainter);
	// stackPanel.addTab("Stack dump", 0);
	
	
	GuiConsole gdbConsole(0.0f, 0.0f);
	GuiConsole infConsole(0.0f, 0.0f);
	gdbConsole.setParent(gui);
	infConsole.setParent(gui);
	
	auto consolePainter = [&](string tabName, void *userData) -> void
	{
		if(tabName == "Debugger")
			gdbConsole.draw();
		else if(tabName == "Inferior")
			infConsole.draw();
	};
	
	GuiTabPanel consolePanel("ConsolePanel", 0.65, 0.33);
	consolePanel.addTab("Debugger", consolePainter);
	consolePanel.addTab("Inferior", consolePainter);
	// GuiConsole console(0.65, 0.33);
	
	auto logUpdateCB = [&](GDBMI * dbg)
	{
		deque<GDBMI::LogItem> logs = dbg->getLogs();
		
		auto MakeColor = [&](uint32_t r, uint32_t g, uint32_t b) -> ImVec4
		{
			return ImVec4(
				static_cast<float>(r) / 255.0,
				static_cast<float>(g) / 255.0,
				static_cast<float>(b) / 255.0,
				1.0
			);
		};
		
		vector<ConsoleItem> tmpList;
		for(auto &log : logs)
		{
			ConsoleItem tmp;
			tmp.tag = log.label;
			tmp.text = log.logText;
			
			using LogLevel = GDBMI::LogLevel;
			switch(log.logLevel)
			{
				// *INDENT-OFF*
				case LogLevel::NeedsFix: tmp.tagColor = MakeColor(255, 52, 52); break;
				case LogLevel::Error: 	 tmp.tagColor = MakeColor(255, 52, 52); break;
				case LogLevel::Warn: 	 tmp.tagColor = MakeColor(255, 145, 48); break;
				case LogLevel::Info: 	 tmp.tagColor = MakeColor(255, 255, 255); break;
				case LogLevel::Verbose:  tmp.tagColor = MakeColor(77, 255, 39); break;
				case LogLevel::Debug: 	 tmp.tagColor = MakeColor(38, 127, 255); break;
				// *INDENT-ON*
			}
			
			tmpList.push_back(tmp);
		}
		
		gdbConsole.clear();
		gdbConsole.addItems(tmpList);
	};
	
	auto inferiorOutputUpdateCB = [&](GDBMI * dbg)
	{
		deque<string> output = dbg->getInferiorOutput();
		
		vector<ConsoleItem> tmpList;
		for(auto &str : output)
		{
			ConsoleItem tmp;
			tmp.text = str;
			
			tmpList.push_back(tmp);
		}
		
		infConsole.clear();
		infConsole.addItems(tmpList);
	};
	
	gdb->setLogUpdateCB(logUpdateCB);
	gdb->setInferiorOutputCB(inferiorOutputUpdateCB);
	
	gui->addChild(&toolbar);
	gui->addChild(&leftPanel);
	gui->addChild(&codeView);
	gui->addChild(&rightPanel);
	// gui->addChild(&gdbConsole);
	gui->addChild(&consolePanel);
	gui->addChild(&stackPanel);
	
	
	GuiMenuBuilder appMenu;
	auto menuBuilder = [&]() -> void
	{
		appMenu.clear();
		
		GuiMenuItem menuItem;
		GuiMenuItem subItem;
		
		menuItem.type = MenuItemType::Submenu;
		menuItem.label = "File";
		{
			subItem = // Open inferior
			{
				MenuItemType::Regular,
				"Open inferior...",
				"Ctrl+O",
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				[](GuiMenuItem * obj) -> void
				{
					gui->showDialog(DialogID::OpenInferior);
				}
			};
			menuItem.subItems.push_back(subItem);
			
			subItem = // Attach to process
			{
				MenuItemType::Regular,
				"Attach to process",
				"",		// Shortcut
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				[](GuiMenuItem * obj) -> void
				{
				
				}
			};
			menuItem.subItems.push_back(subItem);
			
			subItem = // Recently opened files submenu
			{
				MenuItemType::Submenu,
				"Recently opened",
				"",		// Shortcut
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				0		// Callback function
			};
			
			const deque<string> &recentFiles = gui->getRecentFilesList();
			if(recentFiles.size() > 0)
			{
				for(const auto &file : recentFiles)
				{
					GuiMenuItem tmp =
					{
						MenuItemType::Regular,
						file.c_str(),
						"",		// Shortcut
						false,	// Toggled
						true,	// Enabled
						{},		// Vector of submenu items
						[](GuiMenuItem * obj) -> void
						{
							string txtIn = obj->label;
							string txtCmd;
							string txtArg;
							
							if(txtIn.length() > 0)
								gui->addRecentFile(txtIn);
								
							if(txtIn.length() > 0 && txtIn.find(' ') != string::npos)
							{
								size_t pos = txtIn.find(' ');
								txtCmd = txtIn.substr(0, pos);
								txtArg = txtIn.substr(pos + 1);
							}
							else
								txtCmd = txtIn;
								
							gui->setInferiorPathInfo(txtCmd);
							gui->setInferiorArgsInfo(txtArg);
							gdb->doFileCommand(GDBMI::FileCmd::FileExecWithSymbols, txtCmd.c_str());
							gdb->setInferiorArgs(txtArg);
						}
					};
					
					subItem.subItems.push_back(tmp);
				}
			}
			
			menuItem.subItems.push_back(subItem);
			menuItem.subItems.push_back({MenuItemType::Separator});
			
			subItem = // Exit
			{
				MenuItemType::Regular,
				"Exit",
				"Ctrl+Q",		// Shortcut
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				[](GuiMenuItem * obj) -> void
				{
					gui->exit();
				}
			};
			menuItem.subItems.push_back(subItem);
		}
		appMenu.addMenu(menuItem);
		menuItem.subItems.clear();
		
		menuItem.label = "Debug";
		{
			subItem = // Detach from inferior
			{
				MenuItemType::Regular,
				"Detach from inferior",
				"",		// Shortcut
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				[](GuiMenuItem * obj) -> void
				{
					gdb->detachInferior();
				}
			};
			menuItem.subItems.push_back(subItem);
			
			subItem = // Detach and terminate
			{
				MenuItemType::Regular,
				"Detach and terminate",
				"",		// Shortcut
				false,	// Toggled
				true,	// Enabled
				{},		// Vector of submenu items
				[](GuiMenuItem * obj) -> void
				{
					gdb->evaluateExpr("(void)exit(0)");
					gdb->detachInferior();
				}
			};
			menuItem.subItems.push_back(subItem);
		}
		
		appMenu.addMenu(menuItem);
		gui->setMenuBuilder(appMenu);
		gui->setRebuildMenu(false);
	};
	
	gui->setMenuBuilder(appMenu);
	gui->setMenuBuildCallback(menuBuilder);
	gui->setRebuildMenu();
	
	gdb->doFileCommand(GDBMI::FileCmd::FileExecWithSymbols, "/home/aj/code/shell-calc/calc");
	gui->run();
	
	
	
	delete gui;
	delete gdb;
	
	return 0;
}

void symbolTabPainter(string tabName, void *userData)
{
	if(tabName == "Local Func" || tabName == "Imports")
	{
		mutex &funcListMutex = gui->getFuncListMutex();
		
		funcListMutex.lock();
		const vector<GDBMI::SymbolObject> &funcList = gui->getFuncList();
		GDBMI::CurrentInstruction curPos = gdb->getCurrentExecutionPos();
		
		static std::string selectedFunc = "";
		for(uint32_t i = 0; i < funcList.size(); i++)
		{
			// TODO: Fix this cheap-ass method of checking for external symbols
			if(tabName == "Imports")
			{
				if(funcList[i].fullPath.find("/include/") == std::string::npos)
					continue;
			}
			else
			{
				if(funcList[i].fullPath.find("/include/") != std::string::npos)
					continue;
			}
			
			bool funcIsActive = false;
			if(curPos.second.length() > 0 && funcList[i].name.find(curPos.second) != string::npos)
				funcIsActive = true;
				
			if(funcIsActive)
				PushFont(gui->getBoldFont());
				
			string itemText = funcList[i].description;
			if(Selectable((string(" ") + itemText).c_str(), (selectedFunc == itemText)))
				selectedFunc = itemText;
				
			if(funcIsActive)
				PopFont();
				
			if(IsItemHovered())
				SetTooltip("%s", itemText.c_str());
				
			if(IsItemClicked())
			{
				//
				gdb->requestDisassembleLine(funcList[i].shortName, funcList[i].lineNumber);
			}
		}
		
		funcListMutex.unlock();
		
	}
	else if(tabName == "Global Vars")
	{
		mutex &gvarMutex = gui->getGlobVarListMtx();
		
		gvarMutex.lock();
		vector<GDBMI::SymbolObject> &gvarList = gui->getGlobVarList();
		
		static std::string selectedVar = "";
		for(uint32_t i = 0; i < gvarList.size(); i++)
		{
			// if(gvarList[i].fullPath.find("/include/") == std::string::npos)
			// continue;
			
			string itemText = gvarList[i].description;
			if(Selectable((string(" ") + itemText).c_str(), (selectedVar == itemText)))
				selectedVar = itemText;
				
			if(IsItemHovered())
				SetTooltip("%s", itemText.c_str());
				
			if(IsItemClicked())
			{
				// TODO: Handle click input on global variable list
				//
			}
		}
		
		gvarMutex.unlock();
	}
}

void registerTabPainter(string tabName, void *userData)
{
	// TODO: This function is going to need to be able to handle multiple architectures
	
	mutex &regMutex = gui->getRegListMutex();
	vector<GDBMI::RegisterInfo> &regList = gui->getRegList();
	
	vector<string> generalRegisters =
	{
		"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rip", "eflags"
	};
	
	auto isGenReg = [&](string & r) -> bool
	{
		for(auto &genreg : generalRegisters)
		{
			if(genreg == r)
				return true;
		}
		
		return false;
	};
	
	ImFont *boldFont = gui->getBoldFont();
	Columns(2);
	
	ImVec4 ripColor = gui->getColor(GuiItem::RegisterProgCtr);
	ImVec4 chgColor = gui->getColor(GuiItem::RegisterValChg);
	
	regMutex.lock();
	for(auto &reg : regList)
	{
		if(isGenReg(reg.regName))
		{
			SetColumnWidth(-1, 90.0);
			PushFont(boldFont);
			
			Text(reg.regName.c_str());
			
			PopFont();
			NextColumn();
			
			if(reg.regName == "rip")
			{
				PushStyleColor(ImGuiCol_Text, ripColor);
				
				Text(reg.regValue.c_str());
				
				PopStyleColor(1);
			}
			else if(reg.updated == true)
			{
				PushStyleColor(ImGuiCol_Text, chgColor);
				PushFont(boldFont);
				
				Text(reg.regValue.c_str());
				
				PopFont();
				PopStyleColor(1);
			}
			else
				Text(reg.regValue.c_str());
				
			Separator();
			NextColumn();
		}
	}
	
	regMutex.unlock();
	Columns(1);
}

void backtraceTabPainter(string tabname, void *userData)
{
	mutex &btMutex = gui->getBacktraceMutex();
	
	ImFont *boldFont = gui->getBoldFont();
	// ImFont *boldItalicFont = gui->getBoldItalicFont();
	
	// Column headers
	Columns(4);
	SetColumnWidth(0, 80);
	SetColumnWidth(1, 300);
	SetColumnWidth(2, 200);
	SetColumnWidth(3, 80);
	
	PushFont(boldFont);
	Text("Frame #");
	NextColumn();
	Text("Function");
	NextColumn();
	Text("File");
	NextColumn();
	Text("Line #");
	NextColumn();
	Separator();
	PopFont();
	
	btMutex.lock();
	
	vector<GDBMI::FrameInfo> &backtrace = gui->getBacktrace();
	for(auto &frame : backtrace)
	{
		// PushStyleColor(ImGuiCol_Header, gui->getColor(GuiItem::ActiveFrame));
		
		char lvl[16] = {0};
		sprintf(lvl, "%u", frame.level);
		Selectable(lvl, true, ImGuiSelectableFlags_SpanAllColumns);
		NextColumn();
		
		// PopStyleColor(1);
		
		Text(frame.func.c_str());
		NextColumn();
		
		Text(frame.file.c_str());
		NextColumn();
		
		char lineNum[64] = {0};
		sprintf(lineNum, "%u", frame.line);
		Text(lineNum);
		Separator();
		NextColumn();
		
		if(frame.vars.size() > 0)
		{
			Columns(2);
			SetColumnWidth(0, 80);
			
			
			for(auto &var : frame.vars)
			{
				Selectable(" ", false, ImGuiSelectableFlags_SpanAllColumns);
				NextColumn();
				
				char varStr[1024] = {0};
				sprintf(varStr, "%s %s = %s",
						var.type.c_str(),
						var.name.c_str(),
						var.value.c_str());
						
				Text(varStr);
				
				Separator();
				NextColumn();
				
			}
			
			Columns(4);
			SetColumnWidth(0, 80);
			SetColumnWidth(1, 300);
			SetColumnWidth(2, 200);
			SetColumnWidth(3, 80);
		}
	}
	
	Columns(1);
	btMutex.unlock();
}

void breakpointsTabPainter(string tabName, void *userData)
{
	GDBMI::StepFrame stepFrame = gdb->getStepFrame();
	GDBMI::CurrentInstruction curPos = gdb->getCurrentExecutionPos();
	
	mutex &bpMutex = gui->getBreakpointMutex();
	bpMutex.lock();
	
	ImFont *boldFont = gui->getBoldFont();
	vector<GDBMI::BreakpointInfo> &bpList = gui->getBreakpointList();
	
	
	Columns(4);
	SetColumnWidth(0, 60);
	SetColumnWidth(1, 60);
	SetColumnWidth(2, 190);
	// SetColumnWidth(3, 80);
	
	PushFont(boldFont);
	Text("BP #");
	NextColumn();
	Text("Tmp");
	NextColumn();
	Text("Address");
	NextColumn();
	Text("Function");
	NextColumn();
	Separator();
	PopFont();
	
	for(auto &bp : bpList)
	{
		bool bpIsPC = false;
		if(stepFrame.isValid)
		{
			// Is this instruction pointed to by the frame GDB just gave us?
			if(stepFrame.address == bp.addr)
				bpIsPC = true;
		}
		else
		{
			// Is this instruction pointed to by the program counter, $pc (ex. eip, rip)?
			if(curPos.first == strtoull(bp.addr.c_str(), 0, 16))
				bpIsPC = true;
		}
		
		if(bpIsPC)
		{
			PushStyleColor(ImGuiCol_Header, gui->getColor(GuiItem::CodeViewBpBackground));
			PushStyleColor(ImGuiCol_Text, gui->getColor(GuiItem::CodeViewBPisPCText));
		}
		
		char buf[512] = {0};
		sprintf(buf, "%u", bp.number);
		
		Selectable(buf, bpIsPC, ImGuiSelectableFlags_SpanAllColumns);
		NextColumn();
		Text(bp.disp == "keep" ? " No" : "Yes");
		NextColumn();
		Text(bp.addr.c_str());
		NextColumn();
		
		sprintf(buf, "%s:%u", bp.func.c_str(), bp.line);
		Text(buf);
		Separator();
		NextColumn();
		
		if(bpIsPC)
			PopStyleColor(2);
	}
	
	Columns(1);
	bpMutex.unlock();
}
