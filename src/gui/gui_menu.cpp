#include "gui_menu.h"

void GuiMenuItem::draw()
{
	switch(type)
	{
		case MenuItemType::Regular:
		{
			if(MenuItem(label.c_str(), shortcut.c_str(), false, isEnabled) && cbFunc != 0)
				cbFunc(this);
		}
		break;
		
		case MenuItemType::Toggle:
		{
			if(MenuItem(label.c_str(), shortcut.c_str(), &isToggled, isEnabled) && cbFunc != 0)
				cbFunc(this);
		}
		break;
		
		case MenuItemType::Submenu:
		{
			if(BeginMenu(label.c_str(), isEnabled))
			{
				for(auto &item : subItems)
					item.draw();
					
				EndMenu();
			}
		}
		break;
		
		case MenuItemType::Separator:
		{
			Separator();
		}
		break;
	}
}

GuiMenuItem *GuiMenuBuilder::getMenuItem(vector<string> menuPath)
{
	if(menuPath.size() == 0)
		return 0;
		
	uint32_t ctr = 0;
	vector<GuiMenuItem> &curList = m_menuList;
	
	for(auto &label : menuPath)
	{
		GuiMenuItem *findRes = findMenuItem(curList, label);
		ctr++;
		
		if(findRes == 0 || ctr >= menuPath.size())
			return findRes;
			
		curList = findRes->subItems;
	}
	
	return 0;
}

GuiMenuItem *GuiMenuBuilder::findMenuItem(vector<GuiMenuItem> &list, string label)
{
	for(auto &item : m_menuList)
	{
		if(item.label == label)
			return &item;
	}
	
	return 0;
}
