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
