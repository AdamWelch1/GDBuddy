#include "gui_console.h"

GuiConsole::GuiConsole(float w, float h)
{
	m_width = w;
	m_height = h;
}

GuiConsole::~GuiConsole()
{

}

void GuiConsole::draw()
{
	if(m_sameLine)
		SameLine();
		
	ImVec2 mwSize = m_parent->getMainWindowSize();
	
	if(BeginChild("GuiConsole", { mwSize.x * m_width, mwSize.y * m_height }, true))
	{
		m_consoleItemsMutex.lock();
		uint32_t ctr = 0;
		for(auto &item : m_consoleItems)
		{
			TextColored(item.tagColor, item.tag.c_str());
			SameLine();
			Text(item.text.c_str());
			ctr++;
		}
		
		if(m_updateFlag)
		{
			SetScrollHereY();
			m_updateFlag = false;
		}
		
		m_consoleItemsMutex.unlock();
	}
	
	EndChild();
}
