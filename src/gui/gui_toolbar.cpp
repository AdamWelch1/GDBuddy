#include "gui_toolbar.h"

GuiToolbar::GuiToolbar()
{

}

GuiToolbar::~GuiToolbar()
{

}

void GuiToolbar::addButton(ButtonInfo &binfo)
{
	m_buttons.push_back(binfo);
}

void GuiToolbar::draw()
{
	ImGuiIO &io = GetIO();
	BeginGroup();
	{
		uint32_t ctr = 0;
		for(auto &button : m_buttons)
		{
			if(button.isVisible() == false)
				continue;
				
			if(m_parent->isKeyboardAvailable() && !m_parent->isPopupOpen())
			{
				if((Button(button.text.c_str()) || button.chkKeyCmd()) && button.onClick != 0)
					button.onClick();
			}
			else
			{
				if(Button(button.text.c_str()) && button.onClick != 0)
					button.onClick();
			}
			
			if(IsItemHovered())
				SetTooltip(button.tooltip.c_str());
				
			ctr++;
			if(ctr < m_buttons.size())
				SameLine();
		}
	}
	
	if(m_statusCallback != 0)
	{
		SameLine();
		Text(m_statusCallback().c_str());
	}
	
	Separator();
	EndGroup();
}
