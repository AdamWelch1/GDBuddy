#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>

#include <unistd.h>
#include <fcntl.h>

#include "gdb_io.h"
#include "gdbmi/gdbmi.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>

using namespace ImGui;
#define APP_FONT_SIZE			14
#define APP_FONT_DEFAULT		"fonts/Anonymous Pro Minus.ttf"
#define APP_FONT_BOLD			"fonts/Anonymous Pro Minus B.ttf"
#define APP_FONT_ITALICS		"fonts/Anonymous Pro Minus I.ttf"
#define APP_FONT_BOLDITALIC		"fonts/Anonymous Pro Minus BI.ttf"

int32_t gdbPipeIn[2];
int32_t gdbPipeOut[2];
pid_t	gdbPID = 0;
char 	gdbPath[] = "/home/aj/code/official-gdb/gdb_bin/bin/gdb";

void signalHandler(int param)
{
	printf("Signal handler; param = %d\n", param);
}

bool runGDB()
{
	int pipeRes = -1;
	if((pipeRes = pipe(gdbPipeIn)) < 0 || pipe(gdbPipeOut) < 0)
	{
		if(pipeRes >= 0)
		{
			close(gdbPipeIn[0]);
			close(gdbPipeIn[1]);
		}
		
		printf("Failed to allocate pipes for GDB child process\n");
		return false;
	}
	
	pid_t forkRet = fork();
	
	if(forkRet == -1)
	{
		if(pipeRes >= 0)
		{
			close(gdbPipeIn[0]);
			close(gdbPipeIn[1]);
		}
		
		printf("The fork() call failed when trying to launch GDB\n");
		return false;
	}
	
	if(forkRet == 0) // Child process
	{
		dup2(gdbPipeIn[0], STDIN_FILENO);
		dup2(gdbPipeOut[1], STDOUT_FILENO);
		dup2(gdbPipeOut[1], STDERR_FILENO);
		
		close(gdbPipeIn[0]);
		close(gdbPipeIn[1]);
		close(gdbPipeOut[0]);
		close(gdbPipeOut[1]);
		
		int32_t execRet = execl(gdbPath, gdbPath, "--interpreter=mi", "--nx" /* no .gdbinit */, (char *) 0);
		
		exit(execRet);
	}
	else // Parent process - us :)
	{
		close(gdbPipeIn[0]);
		close(gdbPipeOut[1]);
		gdbPID = forkRet;
		
		// int flags[2] = {0};
		// flags[0] = fcntl(gdbPipeIn[1], F_GETFD);
		// flags[1] = fcntl(gdbPipeOut[0], F_GETFD);
		
		// flags[0] |= FD_CLOEXEC;
		// flags[1] |= FD_CLOEXEC;
		
		// fcntl(gdbPipeIn[1], F_SETFD, flags[0]);
		// fcntl(gdbPipeOut[0], F_SETFD, flags[1]);
		
		int nbFlags = fcntl(gdbPipeOut[0], F_GETFL, 0);
		nbFlags |= O_NONBLOCK;
		fcntl(gdbPipeOut[0], F_SETFL, nbFlags);
	}
	
	return true;
}

// bool gdbRead(std::string &buf)
bool gdbRead(std::string &buf)
{
	char readBuf[4096];
	memset(readBuf, 0, 4096);
	
	bool noDataRead = true;
	int32_t readRes = 0;
	int32_t readFailCount = 0;
	
	std::string tmpStr;
	
	while((readRes = read(gdbPipeOut[0], readBuf, 4095)) > 0)
	{
		if(readRes == -1 && errno == EAGAIN)
		{
			if(readFailCount >= 3)
				break;
				
			usleep(1000 * 10);
			continue;
		}
		
		if(readRes == -1 && errno != EAGAIN)
		{
			if(readFailCount >= 3)
				break;
				
			fprintf(stderr, "[%s:%u] read() failed\n", __FILE__, __LINE__);
			perror("Error");
			
			readFailCount++;
			usleep(1000 * 100);
			continue;
		}
		
		noDataRead = false;
		readFailCount = 0;
		
		/*
			if(buf.allocSize <= (buf.numBytes + readRes))
			{
				uint32_t newSize = buf.allocSize;
				while(newSize <= (buf.numBytes + readRes))
					newSize += 4096;
		
				buf.buf = (uint8_t *) realloc(buf.buf, newSize);
				buf.allocSize = newSize;
		
				if(buf.buf == 0)
				{
					printf("realloc() failed to allocate memory!\n");
					fflush(stdout);
					abort();
				}
			}
		*/
		
		// printf("PROC1: %s\n", readBuf);
		// memcpy((buf.buf + buf.numBytes), readBuf, readRes);
		// buf.numBytes += readRes;
		tmpStr.append(readBuf);
		memset(readBuf, 0, 4096);
	}
	
	buf = tmpStr;
	return !(noDataRead);
}

bool gdbWrite(const std::string &buf)
{
	const char *sendBuf = buf.c_str();
	// printf("Raw write: %s\n", sendBuf);
	int32_t sentBytes = 0;
	int32_t writeRes = 0;
	
	while((writeRes = write(gdbPipeIn[1], sendBuf, (buf.length() - sentBytes))) > 0)
	{
		sentBytes += writeRes;
		sendBuf += writeRes;
		usleep(1000 * 10);
	}
	
	if(writeRes <= 0 && errno != EAGAIN)
	{
		fprintf(stderr, "[%s:%u] write() failed\n", __FILE__, __LINE__);
		perror("Error");
	}
	
	if(writeRes < 0 && errno == EAGAIN)
		writeRes = 0;
		
	return (writeRes >= 0);
}

int main(int argc, char **argv)
{
	signal(SIGPIPE, signalHandler);
	GDBMI tst;
	
	while(true)
		usleep(1000 * 100);
		
	return 0;
	
	if(!runGDB())
	{
		printf("Failed to run GDB\n");
		return -1;
	}
	
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
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = 0;
	ImFont *defaultFont [[maybe_unused]]	= io.Fonts->AddFontFromFileTTF(APP_FONT_DEFAULT, APP_FONT_SIZE);
	ImFont *boldFont [[maybe_unused]]		= io.Fonts->AddFontFromFileTTF(APP_FONT_BOLD, APP_FONT_SIZE);
	ImFont *italicFont [[maybe_unused]]		= io.Fonts->AddFontFromFileTTF(APP_FONT_ITALICS, APP_FONT_SIZE);
	ImFont *boldItalicFont [[maybe_unused]]	= io.Fonts->AddFontFromFileTTF(APP_FONT_BOLDITALIC, APP_FONT_SIZE);
	// (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	
	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	int32_t winSize_w, winSize_h;
	SDL_GetWindowSize(window, &winSize_w, &winSize_h);
	SDL_SetWindowPosition(window, 0, 0);
	
	char *consoleBuf = 0;
	uint32_t consoleAlloc = 0;
	const uint32_t ALLOC_SIZE = 16 * 1024; // Our console buffer will grow in 16kb increments
	
	if(consoleBuf == 0) // Allocate console buffer
	{
		consoleBuf = (char *) calloc(1, ALLOC_SIZE);
		consoleAlloc = ALLOC_SIZE;
		
		if(consoleBuf == 0)
		{
			printf("Failed to allocate memory for console!\n");
			fflush(stdout);
			abort();
		}
	}
	
	auto consoleBufResize = [&]() -> uint32_t
	{
		consoleAlloc += ALLOC_SIZE;
		consoleBuf = (char *) realloc(consoleBuf, consoleAlloc);
		
		if(consoleBuf == 0)
		{
			printf("Failed to reallocate memory for console!\n");
			fflush(stdout);
			abort();
		}
		
		return consoleAlloc;
	};
	
	GDBIO gdb(gdbRead, gdbWrite);
	gdb.setConsole(consoleBuf, consoleAlloc, consoleBufResize);
	
	usleep(300 * 1000);
	
	gdb.loadInferior("/home/aj/code/mandelbrot/main");
	// gdb.loadInferior("/usr/bin/galculator");
	gdb.sendCmd("-gdb-set disassembly-flavor intel\n");
	
	float addrColor[4] = {0.000f, 0.748f, 0.856f, 1.000f};
	float instColor[4] = {0.879f, 0.613f, 0.000f, 1.000f};
	float colorHover[4] = {(45.0 / 255.0), (78.0 / 255.0), (106 / 255.0), 1.0};
	float colorColHeader[4] = {0.118f, 0.139f, 0.209f, 1.000f}; // Column header color
	PushStyleColor(ImGuiCol_ChildBg, (ImVec4) ImColor(40, 41, 35));
	PushStyleColor(ImGuiCol_FrameBg, (ImVec4) ImColor(40, 41, 35));
	PushStyleColor(ImGuiCol_Header, (ImVec4) ImColor(56, 56, 48));
	PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(colorHover));
	
	bool exitProgram = false;
	while(!exitProgram)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if(event.type == SDL_QUIT)
				exitProgram = true;
				
			if(event.type == SDL_WINDOWEVENT &&
					event.window.event == SDL_WINDOWEVENT_CLOSE &&
					event.window.windowID == SDL_GetWindowID(window))
				exitProgram = true;
		}
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		
		// Set up MainAppWindow parameters
		ImVec2 mwSize = {(float) winSize_w, (float) winSize_h};
		ImGui::SetNextWindowSize(mwSize);
		ImGui::SetNextWindowPos({0, 0});
		
		ImGuiWindowFlags_ mainWindowFlags = (ImGuiWindowFlags_)
											(ImGuiWindowFlags_NoTitleBar |
											 ImGuiWindowFlags_NoResize |
											 ImGuiWindowFlags_NoMove |
											 ImGuiWindowFlags_MenuBar |
											 ImGuiWindowFlags_NoSavedSettings);
											 
		ImGui::Begin("MainAppWindow", 0, mainWindowFlags);
		
		// Main window menu bar
		if(ImGui::BeginMenuBar())
		{
			if(ImGui::BeginMenu("File"))
			{
				ImGui::MenuItem("Open...");
				if(ImGui::IsItemHovered())
					ImGui::SetTooltip("Open an executable file");
					
				ImGui::MenuItem("Exit");
				if(ImGui::IsItemClicked())
					exitProgram = true;
					
				ImGui::EndMenu();
			}
			
			ImGui::EndMenuBar();
		}
		
		// Toolbar
		BeginGroup();
		{
			ImVec2 buttonSize = {0, 0};
			
			Button("Run", buttonSize);
			SameLine();
			
			if(IsItemClicked())
				gdb.runInferior();
				
			if(gdb.inferiorIsRunning() == false && gdb.inferiorExited() == false)
			{
				Button("Cont", buttonSize);
				if(IsItemClicked())
					gdb.contInferior();
					
			}
			else if(gdb.inferiorIsRunning() == true)
			{
				Button("Pause", buttonSize);
				if(IsItemClicked())
					gdb.pauseInferior();
					
			}
			
			SameLine();
			Button("Stop", buttonSize);
			SameLine();
			Button("Step", buttonSize);
			SameLine();
			Button("Step Inst", buttonSize);
			SameLine();
			Button("Step Over", buttonSize);
			SameLine();
			Button("Step Over Inst", buttonSize);
			SameLine();
			
			Text(gdb.getStatusStr().c_str());
			Separator();
		}
		EndGroup();
		
		ImVec2 availSize = GetContentRegionAvail();
		if(BeginChild("SymbolsPane", {availSize.x * 0.2, availSize.y * 0.6 }, true))
		{
			static ImGuiTabBarFlags_ tabFlags = (ImGuiTabBarFlags_)
												(ImGuiTabBarFlags_Reorderable |
												 ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
												 ImGuiTabBarFlags_FittingPolicyScroll);
												 
			if(BeginTabBar("SymbolsTabList", tabFlags))
			{
				if(BeginTabItem("Functions"))
				{
					gdb.lockSymbols();
					// std::vector<FileSymbolSet> &funcList = gdb.getFunctionSymbols();
					std::vector<FileSymbolSet> &funcList = gdb.getFunctionSymbols();
					
					static std::string selectedFunc = "";
					for(uint32_t i = 0; i < funcList.size(); i++)
					{
						if(funcList[i].fullName.find(gdb.getProjectDir()) == std::string::npos)
							continue;
							
						int32_t activeFuncIndex = funcList[i].activeFunctionIndex;
						for(uint32_t j = 0; j < funcList[i].symbolList.size(); j++)
						{
							std::pair<std::string, std::string> &item = funcList[i].symbolList[j];
							std::string itemTxt = funcList[i].shortName + ":";
							itemTxt += item.first + " ";
							itemTxt += item.second;
							
							if(activeFuncIndex >= 0 && j == activeFuncIndex)
								PushFont(boldFont);
								
							if(Selectable(item.second.c_str(), (selectedFunc == itemTxt)))
								selectedFunc = itemTxt;
								
							if(activeFuncIndex >= 0 && j == activeFuncIndex)
								PopFont();
								
							if(IsItemHovered())
								SetTooltip(itemTxt.c_str());
								
							if(IsItemClicked())
							{
								std::string tmpstr = "-f ";
								tmpstr += (funcList[i].shortName + " -l ") + item.first;
								// size_t pos = tmpstr.find_last_of("/");
								// tmpstr = tmpstr.substr(pos + 1);
								// tmpstr.insert(tmpstr.begin(), '"');
								// tmpstr.insert(tmpstr.end(), '"');
								
								gdb.showDisassembly(tmpstr);
							}
						}
					}
					
					gdb.unlockSymbols();
					EndTabItem();
				}
				
				if(BeginTabItem("Imports"))
				{
					gdb.lockSymbols();
					// std::vector<FileSymbolSet> &funcList = gdb.getFunctionSymbols();
					std::vector<FileSymbolSet> &funcList = gdb.getFunctionSymbols();
					
					static std::string selectedFunc = "";
					for(uint32_t i = 0; i < funcList.size(); i++)
					{
						if(funcList[i].fullName.find(gdb.getProjectDir()) != std::string::npos)
							continue;
							
						for(uint32_t j = 0; j < funcList[i].symbolList.size(); j++)
						{
							std::pair<std::string, std::string> &item = funcList[i].symbolList[j];
							std::string itemTxt = funcList[i].shortName + ":";
							itemTxt += item.first + " ";
							itemTxt += item.second;
							if(Selectable(item.second.c_str(), (selectedFunc == itemTxt)))
								selectedFunc = itemTxt;
								
							if(IsItemHovered())
								SetTooltip(itemTxt.c_str());
						}
					}
					
					gdb.unlockSymbols();
					EndTabItem();
				}
				
				EndTabBar();
			}
			
		}
		EndChild();
		SameLine();
		
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
		PushStyleColor(ImGuiCol_ChildBg, ImVec4(colorColHeader));
		if(BeginChild("CodeViewPane", {availSize.x * 0.6, availSize.y * 0.6}, true))
		{
			static std::string selectedInstr;
			gdb.lockCodeView();
			std::vector<DisasLineInfo> &disasLines = gdb.getDisassmemblyLines();
			
			if(disasLines.size() > 0)
			{
				ImFont *tmpFont = GetFont();
				ImVec2 strSize = tmpFont->CalcTextSizeA(APP_FONT_SIZE, FLT_MAX, FLT_MAX, disasLines[0].addr.c_str());
				// printf("strSize = %f, %f\n", strSize.x, strSize.y);
				
				float winWidth = GetWindowWidth();
				float adj = (200.0 - strSize.x + 3.0) / winWidth;
				PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(adj, 0));
				
				// This adds some space above the column headers
				{
					PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0, -10.0));
					Columns(3);
					SetColumnWidth(-1, 150.0);
					SetColumnWidth(1, 200.0);
					
					NextColumn();
					Text(" ");
					NextColumn();
					Text(" ");
					NextColumn();
					Text(" ");
					
					PopStyleVar(1);
				}
				
				NextColumn();
				PushFont(boldFont);
				Text("Address");
				NextColumn();
				Text("Instruction");
				PopFont();
				NextColumn();
				PopStyleColor(1);
				Columns(1);
				
				PushStyleVar(ImGuiStyleVar_ChildBorderSize, (float) 0.0);
				
				if(BeginChild("DisassemblyInstructionList", ImVec2(0, 0), false, 0))
				{
					Columns(3);
					Separator();
					SetColumnWidth(-1, 150.0);
					SetColumnWidth(1, 200.0);
					for(auto &disLine : disasLines)
					{
						NextColumn();
						bool selItem = (selectedInstr == disLine.addr);
						// Text(disLine.addr.c_str());
						
						if(selItem)
							PushFont(boldFont);
							
							
						PushStyleColor(ImGuiCol_Text, ImVec4(addrColor));
						Selectable(disLine.addr.c_str(), selItem, ImGuiSelectableFlags_SpanAllColumns);
						PopStyleColor(1);
						
						
						if(IsItemClicked())
							selectedInstr = disLine.addr;
							
						NextColumn();
						if(selItem)
							PushStyleColor(ImGuiCol_Text, ImVec4(instColor));
						Text(disLine.instr.c_str());
						if(selItem)
							PopStyleColor(1);
						NextColumn();
						
						if(selItem)
							PopFont();
					}
				}
				EndChild();
				
				PopStyleVar(2);
			}
			else
				PopStyleColor(1);
				
			gdb.unlockCodeView();
		}
		else
			PopStyleColor(1);
		EndChild();
		
		PopStyleVar(1);
		// SameLine();
		
		PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);
		availSize = GetContentRegionAvail();
		availSize.y -= 60.0;
		gdb.lockConsole();
		
		InputTextMultiline("Console", consoleBuf, consoleAlloc, availSize,
						   (ImGuiInputTextFlags_ReadOnly |
							ImGuiInputTextFlags_NoHorizontalScroll), 0, 0, gdb.wasConsoleUpdated());
		gdb.unlockConsole();
		
		static bool inputClear = false;
		static char inputBuf[1024 * 32] = {0};
		
		if(!inputClear)
		{
			inputClear = true;
			memset(inputBuf, 0, (1024 * 32));
			SetNextWindowFocus();
			SetKeyboardFocusHere();
		}
		
		if(InputText("ConsoleCmd", inputBuf, (1024 * 32), (ImGuiInputTextFlags_EnterReturnsTrue)))
		{
			SetKeyboardFocusHere(-1);
			SetWindowFocus("ConsoleCmd");
			std::string inBuf = inputBuf;
			inBuf += "\n";
			gdb.sendCmd(inBuf);
			memset(inputBuf, 0, (1024 * 32));
		}
		
		PopStyleVar(1);
		
		// ImGuiColorEditFlags_ ceFlags = (ImGuiColorEditFlags_)
		// 							   (ImGuiColorEditFlags_NoAlpha |
		// 								ImGuiColorEditFlags_InputRGB |
		// 								ImGuiColorEditFlags_PickerHueBar
		// 							   );
		// SetNextItemWidth(200.0);
		// ColorEdit4("Addr color", addrColor, ceFlags);
		// ColorEdit4("Inst color", instColor, ceFlags);
		
		
		ImGui::End();
		
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
	
	PopStyleColor(3);
	
	
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}
