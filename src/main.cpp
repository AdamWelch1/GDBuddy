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

void symbolTabPainter(string tabName, void *userData);
void registerTabPainter(string tabName, void *userData);

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
										SDL_WINDOW_ALLOW_HIGHDPI |
										SDL_WINDOW_MAXIMIZED);
										
		SDL_Rect displaySize;
		SDL_GetDisplayUsableBounds(0, &displaySize);
		
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
	rightPanel.addTab("Local Vars", 0);
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
		button.tooltip = "Run program from the beginning";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Run); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Pause";
		button.tooltip = "Interrupt program execution";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Interrupt); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Continue";
		button.tooltip = "Continue program execution";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Continue); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step over line";
		button.tooltip = "Step over source line";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Next); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step line";
		button.tooltip = "Step source line";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Step); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step over inst";
		button.tooltip = "Step over instruction";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::NextInstruction); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Step inst";
		button.tooltip = "Step instruction";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::StepInstruction); };
		toolbar.addButton(button), button.onClick = 0;
		
		button.text = "Finish exec";
		button.tooltip = "Execute until current function returns";
		button.onClick = [&]() { gdb->doExecCommand(ExecCmd::Finish); };
		toolbar.addButton(button), button.onClick = 0;
	}
	
	
	
	GuiConsole console(1.0, 0.31);
	
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
		
		console.clear();
		console.addItems(tmpList);
	};
	
	gdb->setLogUpdateCB(logUpdateCB);
	
	gui->addChild(&toolbar);
	gui->addChild(&leftPanel);
	gui->addChild(&codeView);
	gui->addChild(&rightPanel);
	gui->addChild(&console);
	
	
	
	gdb->doFileCommand(GDBMI::FileCmd::FileExecWithSymbols, "/home/aj/code/huffman/main");
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
			if(Selectable(itemText.c_str(), (selectedFunc == itemText)))
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
			if(Selectable(itemText.c_str(), (selectedVar == itemText)))
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
