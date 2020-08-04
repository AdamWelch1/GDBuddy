#include "guimanage.h"

extern GDBMI *gdb;

GuiManager::GuiManager(SDL_Window *sdlWin, SDL_GLContext *glCtx)
{
	if(gdb == 0)
	{
		fprintf(stderr, "ERROR: GuiManager must be created *after* the GDBMI object!\n");
		return;
	}
	
	gdb->setNotifyCallback(GuiManager::updateNotifyCBThunk, this);
	m_cacheThread = thread(GuiManager::cacheHandlerThreadThunk, this);
	
	m_sdlWindow = sdlWin;
	m_glContext = glCtx;
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	m_imGuiCtx = ImGui::CreateContext();
	SetCurrentContext(m_imGuiCtx);
	
	const char *glsl_version = "#version 130";
	ImGui_ImplSDL2_InitForOpenGL(m_sdlWindow, *glCtx);
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	m_io = &(ImGui::GetIO());
	
	// Set this to zero to keep ImGui from creating a config file
	m_io->IniFilename	= 0;
	m_defaultFont		= m_io->Fonts->AddFontFromFileTTF(APP_FONT_DEFAULT, APP_FONT_SIZE);
	m_boldFont			= m_io->Fonts->AddFontFromFileTTF(APP_FONT_BOLD, APP_FONT_SIZE);
	m_italicFont		= m_io->Fonts->AddFontFromFileTTF(APP_FONT_ITALICS, APP_FONT_SIZE);
	m_boldItalicFont	= m_io->Fonts->AddFontFromFileTTF(APP_FONT_BOLDITALIC, APP_FONT_SIZE);
	// (void)io;
	//m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(m_sdlWindow, *m_glContext);
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	SDL_SetWindowPosition(m_sdlWindow, 0, 0);
	
	auto MakeColor = [&](uint32_t r, uint32_t g, uint32_t b, uint32_t a) -> ImVec4
	{
		return ImVec4(
			static_cast<float>(r) / 255.0,
			static_cast<float>(g) / 255.0,
			static_cast<float>(b) / 255.0,
			static_cast<float>(a) / 255.0
		);
	};
	
	m_guiColors[GuiItem::CodeViewAddress] = MakeColor(0, 191, 218, 255);
	m_guiColors[GuiItem::CodeViewInstruction] = MakeColor(160, 160, 160, 255);
	m_guiColors[GuiItem::CodeViewInstructionActive] = MakeColor(224, 156, 0, 255);
	m_guiColors[GuiItem::ChildBackground] = MakeColor(40, 41, 35, 255);
	m_guiColors[GuiItem::ListItemBackgroundSelected] = MakeColor(66, 66, 58, 255);
	m_guiColors[GuiItem::ListItemBackgroundHover] = MakeColor(45, 78, 106, 255);
	m_guiColors[GuiItem::CodeViewColumnHeader] = MakeColor(30, 35, 53, 255);
	m_guiColors[GuiItem::RegisterProgCtr] = MakeColor(0, 226, 13, 255);
	m_guiColors[GuiItem::RegisterValChg] = MakeColor(226, 200, 0, 255);
	m_guiColors[GuiItem::ActiveFrame] = MakeColor(103, 75, 0, 255);
	
	/*
		float addrColor[4] = {0.000f, 0.748f, 0.856f, 1.000f};
		float instColor[4] = {0.879f, 0.613f, 0.000f, 1.000f};
		float colorHover[4] = {(45.0 / 255.0), (78.0 / 255.0), (106 / 255.0), 1.0};
		float colorColHeader[4] = {0.118f, 0.139f, 0.209f, 1.000f}; // Column header color
		PushStyleColor(ImGuiCol_ChildBg, (ImVec4) ImColor(40, 41, 35));
		PushStyleColor(ImGuiCol_FrameBg, (ImVec4) ImColor(40, 41, 35));
		PushStyleColor(ImGuiCol_Header, (ImVec4) ImColor(56, 56, 48));
		PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(colorHover));
	*/
}

GuiManager::~GuiManager()
{
	m_exitProgram = true;
	m_cacheThread.join();
}

void GuiManager::run()
{
	mainLoop();
}

void GuiManager::mainLoop()
{
	int32_t winSize_w, winSize_h;
	SDL_GetWindowSize(m_sdlWindow, &winSize_w, &winSize_h);
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	winSize_h -= 28;
	
	while(!m_exitProgram)
	{
		// Input must be grabbed before the NewFrame() call
		handleInput();
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(m_sdlWindow);
		ImGui::NewFrame();
		
		// Set up MainAppWindow parameters
		ImVec2 mwSize = {(float) winSize_w, (float) winSize_h};
		ImGui::SetNextWindowSize(mwSize);
		ImGui::SetNextWindowPos({0, 0});
		
		renderFrame();
		
		ImGui::Render();
		glViewport(0, 0, (int)m_io->DisplaySize.x, (int)m_io->DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(m_sdlWindow);
		
		// usleep(1000 * 30);
	}
}

void GuiManager::renderFrame()
{
	ImGuiWindowFlags_ mainWindowFlags = (ImGuiWindowFlags_)
										(ImGuiWindowFlags_NoTitleBar |
										 ImGuiWindowFlags_NoResize |
										 ImGuiWindowFlags_NoMove |
										 ImGuiWindowFlags_MenuBar |
										 ImGuiWindowFlags_NoSavedSettings);
	//
	
	ImGui::Begin("MainAppWindow", 0, mainWindowFlags);
	// m_mainWindowSize = GetWindowSize();
	m_mainWindowSize = GetContentRegionMax();
	float xStart = m_mainWindowSize.x;
	m_mainWindowSize.x -= 8.0f; // Compensate for child-window padding
	
	PushStyleColor(ImGuiCol_ChildBg, getColor(GuiItem::ChildBackground));
	
	PushStyleColor(ImGuiCol_Header, getColor(GuiItem::ListItemBackgroundSelected));
	PushStyleColor(ImGuiCol_HeaderHovered, getColor(GuiItem::ListItemBackgroundHover));
	PushStyleColor(ImGuiCol_HeaderActive, getColor(GuiItem::ListItemBackgroundHover));
	
	uint32_t ctr = 0;
	for(auto &child : m_guiChildren)
	{
		if(child->isSameLine() == false && child != m_guiChildren[0])
			m_mainWindowSize.x = xStart - 8.0f; // Compensate for child-window padding
			
		if(child->isSameLine() && (ctr + 1) < m_guiChildren.size() && m_guiChildren[ctr + 1]->isSameLine() == false)
			m_mainWindowSize.x -= 32.0f;
			
		if((ctr + 1) >= m_guiChildren.size())
			m_mainWindowSize.x -= 16.0f;
			
		child->draw();
		m_mainWindowSize.x -= 8.0f;
		
		ctr++;
	}
	
	PopStyleColor(4);
	
	m_mainWindowSize.x = xStart;
	ImGui::End();
}

void GuiManager::handleInput()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if(event.type == SDL_QUIT)
			m_exitProgram = true;
			
		if(event.type == SDL_WINDOWEVENT &&
				event.window.event == SDL_WINDOWEVENT_CLOSE &&
				event.window.windowID == SDL_GetWindowID(m_sdlWindow))
			m_exitProgram = true;
	}
}

void GuiManager::updateNotifyCB(GDBMI::UpdateType updateType)
{
	using LogLevel = GDBMI::LogLevel;
	using UpdateType = GDBMI::UpdateType;
	
	m_cacheFlagMutex.lock();
	
	uint64_t updType = (uint64_t) updateType;
	if((updType & (uint64_t) UpdateType::FuncSymbols) != 0)
		setCacheFlag(FLAG_FUNC_CACHE_STALE);
		
	if((updType & (uint64_t) UpdateType::GVarSymbols) != 0)
		setCacheFlag(FLAG_GLOBVAR_CACHE_STALE);
		
	if((updType & (uint64_t) UpdateType::Disassembly) != 0)
		setCacheFlag(FLAG_DISASM_CACHE_STALE);
		
	if((updType & (uint64_t) UpdateType::RegisterInfo) != 0)
		setCacheFlag(FLAG_REGISTER_CACHE_STALE);
		
	if((updType & (uint64_t) UpdateType::Backtrace) != 0)
		setCacheFlag(FLAG_STACKTRACE_CACHE_STALE);
		
	m_cacheFlagMutex.unlock();
}

void GuiManager::cacheHandlerThread()
{
	using LogLevel = GDBMI::LogLevel;
	
	while(!m_exitProgram)
	{
		m_cacheFlagMutex.lock();
		
		if(isFlagSet(FLAG_FUNC_CACHE_STALE))
		{
			m_funcListMutex.lock();
			
			m_funcSymbolCache.clear();
			m_funcSymbolCache = gdb->getFunctionSymbols();
			
			clearCacheFlag(FLAG_FUNC_CACHE_STALE);
			m_funcListMutex.unlock();
		}
		
		if(isFlagSet(FLAG_GLOBVAR_CACHE_STALE))
		{
			m_gvarListMutex.lock();
			
			m_globalVarCache.clear();
			m_globalVarCache = gdb->getGlobalVarSymbols();
			
			clearCacheFlag(FLAG_GLOBVAR_CACHE_STALE);
			m_gvarListMutex.unlock();
		}
		
		if(isFlagSet(FLAG_DISASM_CACHE_STALE))
		{
			m_codeLinesMutex.lock();
			
			vector<GDBMI::DisassemblyInstruction> gdbDisasm = gdb->getDisassembly();
			m_codeLines.clear();
			
			uint32_t ctr = 0;
			for(auto &inst : gdbDisasm)
			{
				AsmLineDesc tmp;
				tmp.addr = inst.addrStr;
				tmp.instr = inst.instruction;
				
				if(ctr++ % 10 == 0)
				{
					tmp.instr += " ";
					tmp.instr += inst.funcName + "<";
					tmp.instr += inst.offset + ">";
				}
				
				m_codeLines.push_back(tmp);
			}
			
			clearCacheFlag(FLAG_DISASM_CACHE_STALE);
			m_codeLinesMutex.unlock();
		}
		
		if(isFlagSet(FLAG_REGISTER_CACHE_STALE))
		{
			m_registerCacheMutex.lock();
			
			m_registerCache.clear();
			m_registerCache = gdb->getRegisters();
			
			clearCacheFlag(FLAG_REGISTER_CACHE_STALE);
			m_registerCacheMutex.unlock();
		}
		
		if(isFlagSet(FLAG_STACKTRACE_CACHE_STALE))
		{
			m_backtraceMutex.lock();
			
			m_backtraceCache.clear();
			m_backtraceCache = gdb->getBacktrace();
			
			clearCacheFlag(FLAG_STACKTRACE_CACHE_STALE);
			m_backtraceMutex.unlock();
		}
		
		m_cacheFlagMutex.unlock();
		usleep(1000 * 50);
	}
}
