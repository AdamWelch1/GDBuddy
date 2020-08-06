#ifndef UNIQUE_GUI_MENU_H
#define UNIQUE_GUI_MENU_H

#include "gui_defs.h"
#include "gui_child.h"

struct GuiMenuItem;
// typedef void (*MenuItemCallback)(GuiMenuItem *);
typedef function<void(GuiMenuItem *)> MenuItemCallback;

enum class MenuItemType : uint32_t
{
	Regular = 0,
	Toggle,
	Submenu,
	Separator
};

struct GuiMenuItem
{
	MenuItemType type = MenuItemType::Regular;
	string label = "Menu Item";
	string shortcut = "";
	bool isToggled = false;
	bool isEnabled = true;
	vector<GuiMenuItem> subItems;
	MenuItemCallback cbFunc = 0;
	
	// This will hold menuitem-specific data that
	//   will be decoded by the callback handler.
	// I chose this over a void pointer to some
	//   malloc'd buffer to avoid a bunch of
	//   small allocations and deallocations.
	// This should run faster, but structures
	//   must not exceed the size of rawStruct and
	//   this method uses more memory overall.
	uint8_t rawStruct[256] = {0};
	
	void draw();
};

class GuiMenuBuilder
{
	public:
	
		GuiMenuBuilder() {}
		~GuiMenuBuilder() {}
		
		void buildMenu()
		{
			for(auto &menu : m_menuList)
				menu.draw();
		}
		
		void addMenu(GuiMenuItem &gmi)
		{
			if(gmi.type == MenuItemType::Submenu)
				m_menuList.push_back(gmi);
		}
		
	private:
	
		vector<GuiMenuItem> m_menuList;
};

#endif
