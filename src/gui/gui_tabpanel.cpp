#include "gui_tabpanel.h"

GuiTabPanel::GuiTabPanel(string name, float width, float height, int32_t x, int32_t y)
{
	m_panelName = name;
	m_width = width;
	m_height = height;
	m_xPos = x;
	m_yPos = y;
}

void GuiTabPanel::addTab(string tabName,
						 gui_drawcb_t tabDrawCB,
						 bool isSelected,
						 void *userData,
						 ImGuiTabItemFlags tabFlags)
{
	TabContext item;
	item.name = tabName;
	item.callback = tabDrawCB;
	item.userData = userData;
	item.flags = tabFlags;
	item.isSelected = isSelected;
	item.isHidden = false;
	
	m_tabList.push_back(item);
}

void GuiTabPanel::hideTab(string tabName, bool hide)
{
	for(auto &tab : m_tabList)
	{
		if(tabName == tab.name)
		{
			tab.isHidden = hide;
			// Not breaking here because we want to
			// effect multiple tabs with the same name.
		}
	}
}

void GuiTabPanel::draw()
{
	if(m_sameLine)
		SameLine();
		
	ImVec2 mwSize = m_parent->getMainWindowSize();
	
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
	if(BeginChild(string(m_panelName + "panel").c_str(), {mwSize.x * m_width, mwSize.y * m_height}, true))
	{
		PopStyleVar(1);
		static ImGuiTabBarFlags_ tabFlags = (ImGuiTabBarFlags_) \
											(ImGuiTabBarFlags_Reorderable |
											 ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
											 ImGuiTabBarFlags_FittingPolicyScroll);
											 
		if(BeginTabBar(string(m_panelName + "tablist").c_str(), tabFlags))
		{
			for(auto &tab : m_tabList)
			{
				if(tab.isHidden)
					continue;
					
				if(BeginTabItem(tab.name.c_str()))
				{
					if(tab.callback != 0)
						tab.callback(tab.name, tab.userData);
						
					EndTabItem();
				}
			}
			EndTabBar();
		}
		
	}
	else
		PopStyleVar(1);
	EndChild();
	
}











