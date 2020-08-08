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
	m_io->ConfigFlags	|= ImGuiConfigFlags_NavNoCaptureKeyboard;
	// (void)io;
	//m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	loadRecentFiles();
	
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
	
	m_guiColors[GuiItem::CodeViewAddress] = MakeColor(0, 191, 218, 255);			// Regular address color
	m_guiColors[GuiItem::CodeViewAddressBP] = MakeColor(231, 0, 0, 255);
	m_guiColors[GuiItem::CodeViewInstruction] = MakeColor(160, 160, 160, 255);		// Instruction text color
	m_guiColors[GuiItem::CodeViewInstructionActive] = MakeColor(224, 156, 0, 255);	// Active instruction text color
	m_guiColors[GuiItem::CodeViewColumnHeader] = MakeColor(30, 35, 53, 255);		// Column header bg color
	m_guiColors[GuiItem::CodeViewBpBackground] = MakeColor(16, 16, 16, 255);		// Disas. BP line BG color
	m_guiColors[GuiItem::CodeViewBPisPCText] = MakeColor(5, 221, 0, 255);			// Inst. color where BPaddr==$pc
	
	m_guiColors[GuiItem::ChildBackground] = MakeColor(40, 41, 35, 255);				// Child window bg color
	m_guiColors[GuiItem::ListItemBackgroundSelected] = MakeColor(66, 66, 58, 255);	// Selected list item bg color
	m_guiColors[GuiItem::ListItemBackgroundHover] = MakeColor(45, 78, 106, 255);	// List item hover bg color
	m_guiColors[GuiItem::RegisterProgCtr] = MakeColor(0, 226, 13, 255);				// Text color of $pc reg. value
	m_guiColors[GuiItem::RegisterValChg] = MakeColor(226, 200, 0, 255);				// Text color of changed registers
	m_guiColors[GuiItem::ActiveFrame] = MakeColor(103, 75, 0, 255);					// B.trace sel. frame bg color
	
	m_guiColors[GuiItem::TabFocused] = MakeColor(58, 66, 78, 255);
	m_guiColors[GuiItem::TabUnfocused] = MakeColor(58, 66, 78, 255);
	m_guiColors[GuiItem::TabHover] = MakeColor(60, 97, 160, 255);
	m_guiColors[GuiItem::TabActive] = MakeColor(48, 78, 128, 255);
	
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
	// winSize_h -= 28;
	
	while(!m_exitProgram)
	{
		// Dear ImGui suggests doing this before the NewFrame() call, so... here we go
		m_wantCaptureKeyboard	= GetIO().WantCaptureKeyboard;
		m_wantCaptureMouse		= GetIO().WantCaptureMouse;
		
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
	if(isKeyboardAvailable())
	{
		if(isKeyPressed(KEY_CTRL) && isKeyPressed('o'))
			m_showLoadInferiorDialog = true;
			
		if(isKeyPressed(KEY_CTRL) && isKeyPressed('q'))
			m_exitProgram = true;
	}
	
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
	
	PushStyleColor(ImGuiCol_Tab, getColor(GuiItem::TabFocused));
	PushStyleColor(ImGuiCol_TabHovered, getColor(GuiItem::TabHover));
	PushStyleColor(ImGuiCol_TabActive, getColor(GuiItem::TabActive));
	PushStyleColor(ImGuiCol_TabUnfocused, getColor(GuiItem::TabUnfocused));
	PushStyleColor(ImGuiCol_TabUnfocusedActive, getColor(GuiItem::TabFocused));
	
	if(BeginMenuBar())
	{
		if(m_rebuildMenu && m_menuBuildCB != 0)
			m_menuBuildCB();
			
		m_menuBuilder.drawMenu();
		EndMenuBar();
	}
	
	if(m_showLoadInferiorDialog)
	{
		OpenPopup("Openinferior");
		clearKeyPress(KEY_CTRL);
		clearKeyPress('o');
	}
	
	drawDialogs();
	
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
	
	PopStyleColor(9);
	
	m_mainWindowSize.x = xStart;
	ImGui::End();
}

void GuiManager::drawDialogs()
{
	ImVec2 center(GetIO().DisplaySize.x * 0.5f, GetIO().DisplaySize.y * 0.15f);
	SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
	
	if(BeginPopupModal("Openinferior", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		m_popupIsOpen = true;
		
		static char textInputBuf[4096] = {0};
		static bool inputApply = false;
		
		Text("Optionally, you may specify the program arguments here as well");
		Text("Inferior path: ");
		SameLine();
		
		if(IsWindowAppearing())
			SetKeyboardFocusHere(); // Sets keyboard focus to next widget
			
		SetNextItemWidth(500.0f);
		if(InputText("Inferiorpath", textInputBuf, 4095, ImGuiInputTextFlags_EnterReturnsTrue))
			inputApply = true;
			
		m_showLoadInferiorDialog = false;
		SameLine();
		
		if(inputApply || Button("Okay"))
		{
			inputApply = false;
			string txtIn = textInputBuf;
			string txtCmd;
			string txtArg;
			
			if(txtIn.length() > 0)
				addRecentFile(txtIn);
				
			if(txtIn.length() > 0 && txtIn.find(' ') != string::npos)
			{
				size_t pos = txtIn.find(' ');
				txtCmd = txtIn.substr(0, pos);
				txtArg = txtIn.substr(pos + 1);
			}
			else
				txtCmd = txtIn;
				
			m_inferiorInfo.path = txtCmd;
			m_inferiorInfo.args = txtArg;
			
			gdb->doFileCommand(GDBMI::FileCmd::FileExecWithSymbols, txtCmd.c_str());
			gdb->setInferiorArgs(txtArg);
			memset(textInputBuf, 0, 4096);
			CloseCurrentPopup();
			m_popupIsOpen = false;
		}
		
		SameLine();
		
		bool escape = false;
		if(m_keypressMap[KEY_ESCAPE])
			escape = true;
			
		if(escape || Button("Cancel"))
		{
			if(escape)
				m_keypressMap[KEY_ESCAPE] = false;
				
			CloseCurrentPopup();
			memset(textInputBuf, 0, 4096);
			m_popupIsOpen = false;
		}
		
		if(m_recentFiles.size() > 0)
		{
			Text("Recently opened:");
			
			for(auto &file : m_recentFiles)
			{
				Selectable(file.c_str(),
						   false,
						   (ImGuiSelectableFlags_DontClosePopups |
							ImGuiSelectableFlags_AllowDoubleClick));
							
				if(IsItemClicked())
				{
					strcpy(textInputBuf, file.c_str());
					
					if(IsMouseDoubleClicked(0)) // 0 = left button in ImGui...
						inputApply = true;
				}
			}
		}
		
		EndPopup();
	}
}

void GuiManager::showDialog(DialogID dlg)
{
	switch(dlg)
	{
		case DialogID::OpenInferior:
		{
			m_showLoadInferiorDialog = true;
		}
		break;
	}
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
			
		// On my system at least, the maximized SDL window
		// likes to re-appear a few hundred pixels offset from
		// origin 0,0. This fixes that.
		if(event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
		{
			SDL_Keysym keysym = event.key.keysym;
			//
			auto isShortcutChar = [&](uint32_t c) -> bool
			{
				if(c >= SDLK_F1 && c <= SDLK_F12)
					return true;
					
				if(c < 32 || c > 126)
					return false;
					
				char check = (char) c;
				char scChars[] = "abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-=_+,./<>?;:[]{}\\| ";
				
				for(uint32_t i = 0; i < strlen(scChars); i++)
				{
					if(check == scChars[i])
						return true;
				}
				
				return false;
			};
			
			switch(keysym.sym)
			{
				case SDLK_RCTRL:
				case SDLK_LCTRL:
				case SDLK_RSHIFT:
				case SDLK_LSHIFT:
				case SDLK_LALT:
				case SDLK_RALT:
				{
					if(event.key.state == SDL_RELEASED)
					{
						if(keysym.sym == SDLK_RCTRL || keysym.sym == SDLK_LCTRL)
							m_keypressMap[KEY_CTRL] = false;
							
						if(keysym.sym == SDLK_RSHIFT || keysym.sym == SDLK_LSHIFT)
							m_keypressMap[KEY_SHIFT] = false;
							
						if(keysym.sym == SDLK_RALT || keysym.sym == SDLK_LALT)
							m_keypressMap[KEY_ALT] = false;
					}
					else if(event.key.repeat == 0)
					{
						if(keysym.sym == SDLK_RCTRL || keysym.sym == SDLK_LCTRL)
							m_keypressMap[KEY_CTRL] = true;
							
						if(keysym.sym == SDLK_RSHIFT || keysym.sym == SDLK_LSHIFT)
							m_keypressMap[KEY_SHIFT] = true;
							
						if(keysym.sym == SDLK_RALT || keysym.sym == SDLK_LALT)
							m_keypressMap[KEY_ALT] = true;
					}
				}
				break;
				
				case SDLK_ESCAPE:
				{
					if(event.key.state == SDL_RELEASED)
						m_keypressMap[KEY_ESCAPE] = false;
					else if(event.key.repeat == 0)
						m_keypressMap[KEY_ESCAPE] = true;
				}
				break;
			}
			
			if(isShortcutChar(keysym.sym))
			{
				if(event.key.state == SDL_RELEASED)
				{
					// if(m_keypressMap[keysym.sym] == true)
					// gdb->logPrintf(GDBMI::LogLevel::Debug,
					// 			   "Key released: %c (CTRL=%u ALT=%u SHIFT=%u)\n",
					// 			   (char) keysym.sym,
					// 			   m_keypressMap[KEY_CTRL],
					// 			   m_keypressMap[KEY_ALT],
					// 			   m_keypressMap[KEY_SHIFT]);
					m_keypressMap[keysym.sym] = false;
				}
				else if(event.key.repeat == 0)
				{
					// if(m_keypressMap[keysym.sym] == false)
					// gdb->logPrintf(GDBMI::LogLevel::Debug,
					// 			   "Key pressed: %c (CTRL=%u ALT=%u SHIFT=%u)\n",
					// 			   (char) keysym.sym,
					// 			   m_keypressMap[KEY_CTRL],
					// 			   m_keypressMap[KEY_ALT],
					// 			   m_keypressMap[KEY_SHIFT]);
					m_keypressMap[keysym.sym] = true;
				}
				
				if((keysym.mod & KMOD_CTRL) != 0)
					m_keypressMap[KEY_CTRL] = true;
				else
					m_keypressMap[KEY_CTRL] = false;
					
				if((keysym.mod & KMOD_ALT) != 0)
					m_keypressMap[KEY_ALT] = true;
				else
					m_keypressMap[KEY_ALT] = false;
					
				if((keysym.mod & KMOD_SHIFT) != 0)
					m_keypressMap[KEY_SHIFT] = true;
				else
					m_keypressMap[KEY_SHIFT] = false;
			}
		}
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
		
	if((updType & (uint64_t) UpdateType::BreakPtList) != 0)
		setCacheFlag(FLAG_BREAKPOINT_CACHE_STALE);
		
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
		
		if(isFlagSet(FLAG_BREAKPOINT_CACHE_STALE))
		{
			m_bpCacheMutex.lock();
			
			m_breakpointCache.clear();
			m_breakpointCache = gdb->getBpList();
			
			clearCacheFlag(FLAG_BREAKPOINT_CACHE_STALE);
			m_bpCacheMutex.unlock();
		}
		
		m_cacheFlagMutex.unlock();
		usleep(1000 * 50);
	}
}

bool GuiManager::isKeyPressed(uint32_t key)
{
	if(m_keypressMap.find(key) == m_keypressMap.end())
		return false;
		
	return m_keypressMap[key];
}

void GuiManager::clearKeyPress(uint32_t key)
{
	m_keypressMap[key] = false;
}

void GuiManager::loadRecentFiles()
{
	FILE *fp = fopen("./.recent_files.gdbuddy", "rb");
	
	if(fp)
	{
		fprintf(stderr, "********** Loading recent files\n");
		char readBuf[1024] = {0};
		while(!feof(fp))
		{
			memset(readBuf, 0, 1024);
			char *readRes = fgets(readBuf, 1024, fp);
			
			if(readRes)
			{
				string tmp = readRes;
				
				size_t nlPos = tmp.find('\n');
				while(nlPos != string::npos)
				{
					tmp.erase(tmp.begin() + nlPos);
					nlPos = tmp.find('\n');
				}
				
				m_recentFiles.push_back(tmp);
			}
		}
		
		fclose(fp);
	}
}

void GuiManager::saveRecentFiles()
{
	FILE *fp = fopen("./.recent_files.gdbuddy", "wb");
	
	if(fp)
	{
		fprintf(stderr, "********** Saving recent files\n");
		for(auto str : m_recentFiles)
		{
			str += "\n";
			fwrite(str.c_str(), str.length(), 1, fp);
		}
		
		fflush(fp);
		fclose(fp);
	}
}

void GuiManager::addRecentFile(string file)
{
	fprintf(stderr, "********** Adding a recent file\n");
	auto findRecent = [&](const string & str) -> int32_t
	{
		if(str.length() == 0)
			return -1;
			
		for(uint32_t i = 0; i < m_recentFiles.size(); i++)
		{
			if(m_recentFiles[i] == str)
				return i;
		}
		
		return -1;
	};
	
	size_t nlPos = file.find('\n');
	while(nlPos != string::npos)
	{
		file.erase(file.begin() + nlPos);
		nlPos = file.find('\n');
	}
	
	int32_t findRes = findRecent(file);
	while(findRes != -1)
	{
		m_recentFiles.erase(m_recentFiles.begin() + findRes);
		findRes = findRecent(file);
	}
	
	m_recentFiles.push_front(file);
	saveRecentFiles();
	m_rebuildMenu = true;
}
