#ifndef UNIQUE_GUI_DEFS
#define UNIQUE_GUI_DEFS

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <functional>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>

using namespace ImGui;
using std::string;
using std::vector;
using std::thread;
using std::mutex;
using std::map;
using std::function;

#define APP_FONT_SIZE			16
#define APP_FONT_DEFAULT		"fonts/Anonymous Pro Minus.ttf"
#define APP_FONT_BOLD			"fonts/Anonymous Pro Minus B.ttf"
#define APP_FONT_ITALICS		"fonts/Anonymous Pro Minus I.ttf"
#define APP_FONT_BOLDITALIC		"fonts/Anonymous Pro Minus BI.ttf"

#define KEY_CTRL	0
#define KEY_ALT		1
#define KEY_SHIFT	2
#define KEY_ESCAPE	3

enum class DialogID : uint32_t
{
	OpenInferior = 100
};

enum class GuiItem : uint32_t
{
	Text,
	Background,
	ChildBackground,
	
	ListItemText,
	ListItemTextSelected,
	ListItemTextHover,
	ListItemBackgroundSelected,
	ListItemBackgroundHover,
	
	CodeViewColumnHeader,
	CodeViewAddress,
	CodeViewAddressBP,
	CodeViewInstruction,
	CodeViewInstructionActive,
	CodeViewBackground,
	CodeViewBpBackground,
	CodeViewBPisPCText,
	
	RegisterProgCtr,
	RegisterValChg,
	
	ActiveFrame,
	
	TabFocused,
	TabUnfocused,
	TabHover,
	TabActive
	
	/*
		ImGuiCol_Tab,
		ImGuiCol_TabHovered,
		ImGuiCol_TabActive,
		ImGuiCol_TabUnfocused,
		ImGuiCol_TabUnfocusedActive,
	*/
	
};

struct Printable
{
	public:
	
		virtual string toString()
		{
			update();
			
			if(cols.size() == 0)
				return "";
				
			string ret = "";
			int ctr = 0;
			for(auto &i : cols)
			{
				if(ctr++ > 0)
					ret += " ";
				ret += i;
			}
			
			ret += "\n";
			return ret;
		}
		
		virtual string &operator[](uint32_t index)
		{
			if(index >= cols.size())
				return __badref;
				
			return cols[index];
		}
		
		// Updates 'cols' with information about the structure
		virtual void update() = 0;
		
		
	protected:
	
		vector<string> cols;
		string __badref = "!!BadReference!!";
};

struct InferiorInfo : public Printable
{
	string path;
	string args;
	
	void update()
	{
		cols.clear();
		cols.push_back("Inferior path: ");
		cols.push_back(path + "\n");
		cols.push_back("Inferior Args: ");
		cols.push_back(args + "\n");
	}
};

struct AsmLineDesc : public Printable
{
	string addr = "";
	string instr = "";
	bool selected = false;
	bool beingExecuted = false; // True if this instruction's address is in the program counter
	
	void update()
	{
		cols.clear();
		cols.push_back(addr);
		cols.push_back(instr);
	}
};

struct ButtonInfo : public Printable
{
	string text = "Button";
	string tooltip = "";
	function<bool()> isVisible = []() -> bool { return true; };
	function<bool()> chkKeyCmd = []() -> bool { return false; };
	function<void()> onClick = 0;
	
	void update()
	{
		cols.clear();
		cols.push_back(string("Button: ") + text);
		
		if(tooltip.length() > 0)
			cols.push_back(string("(") + tooltip + string(")"));
			
		string isvis = "IsVisible() = ";
		isvis += isVisible.target_type().name();
		isvis += " ;;";
		cols.push_back(isvis);
		
		string onclick = "OnClick() = ";
		onclick += onClick.target_type().name();
		onclick += " ;;";
		cols.push_back(onclick);
	}
};

typedef vector<AsmLineDesc> AsmDump;





#endif
