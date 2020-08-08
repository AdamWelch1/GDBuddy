#ifndef UNIQUE_GUI_TABPANEL_H
#define UNIQUE_GUI_TABPANEL_H

#include "gui_defs.h"
#include "gui_child.h"

typedef function<void(string, void *)> gui_drawcb_t;
// typedef void (*gui_drawcb_t)(string tabName, void *userData);

class GuiTabPanel : public GuiChild
{
	public:
	
		GuiTabPanel(string name, float width, float height, int32_t x = -1, int32_t y = -1);
		~GuiTabPanel() {}
		
		void addTab(string tabName,
					gui_drawcb_t tabDrawCB,
					bool isSelected = false,
					void *userData = 0,
					ImGuiTabItemFlags tabFlags = 0);
					
		void hideTab(string tabName, bool hide);
		
		void draw();
		
	private:
	
		struct TabContext
		{
			string name = "";
			gui_drawcb_t callback = 0;
			ImGuiTabItemFlags flags;
			void *userData = 0;
			bool isSelected = false;
			bool isHidden = false;
		};
		
		vector<TabContext> m_tabList;
		string m_panelName;
		int32_t m_xPos, m_yPos;
		float m_width, m_height;
};

#endif
