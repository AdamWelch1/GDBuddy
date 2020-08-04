#ifndef UNIQUE_GUI_CONSOLE_H
#define UNIQUE_GUI_CONSOLE_H

#include "gui_defs.h"
#include "gui_child.h"

struct ConsoleItem
{
	string tag;
	ImVec4 tagColor;
	string text;
};

class GuiConsole : public GuiChild
{
	public:
	
		GuiConsole(float w, float h);
		~GuiConsole();
		
		void clear()
		{
			m_consoleItemsMutex.lock();
			m_consoleItems.clear();
			m_consoleItemsMutex.unlock();
		}
		
		void addItem(ConsoleItem &ci)
		{
			m_consoleItemsMutex.lock();
			m_consoleItems.push_back(ci);
			m_updateFlag = true;
			m_consoleItemsMutex.unlock();
		}
		
		void addItems(vector<ConsoleItem> &ci)
		{
			m_consoleItemsMutex.lock();
			
			for(auto &iter : ci)
				m_consoleItems.push_back(iter);
			m_updateFlag = true;
			m_consoleItemsMutex.unlock();
		}
		
		void draw();
		
	private:
	
		float m_width = 0.0;
		float m_height = 0.0;
		
		bool m_updateFlag = false;;
		mutex m_consoleItemsMutex;
		vector<ConsoleItem> m_consoleItems;
		
};

#endif
