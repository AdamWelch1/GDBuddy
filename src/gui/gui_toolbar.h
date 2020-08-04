#ifndef UNIQUE_GUI_TOOLBAR_H
#define UNIQUE_GUI_TOOLBAR_H

#include "gui_defs.h"
#include "gui_child.h"

class GuiToolbar : public GuiChild
{
	public:
		GuiToolbar();
		~GuiToolbar();
		
		void setStatusCB(function<string()> cb) { m_statusCallback = cb; }
		void addButton(ButtonInfo &binfo);
		void draw();
		
	private:
	
		vector<ButtonInfo> m_buttons;
		function<string()> m_statusCallback = 0;
};

#endif
