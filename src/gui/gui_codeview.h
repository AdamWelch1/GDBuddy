#ifndef UNIQUE_GUI_CODEVIEW_H
#define UNIQUE_GUI_CODEVIEW_H

#include "gui_defs.h"
#include "gui_child.h"
#include "gui_menu.h"


class GuiCodeView : public GuiChild
{
	public:
	
	
		GuiCodeView();
		GuiCodeView(float w, float h) : GuiCodeView()
		{
			m_width = w;
			m_height = h;
		}
		
		~GuiCodeView() {}
		
		void draw();
		void setFonts(ImFont *defaultFont, ImFont *bold, ImFont *italic, ImFont *bolditalic)
		{
			m_defaultFont 		= defaultFont;
			m_boldFont 			= bold;
			m_italicFont 		= italic;
			m_boldItalicFont 	= bolditalic;
		}
		
	private:
	
		struct GMI_Data
		{
			bool bpIsSet = false;
			uint32_t bpNum = 0x7FFFFFF;
			char setAddr[64] = {0};
		};
		
		void contextMenuHandler(GMI_Data &menuData);
		vector<GuiMenuItem> m_menuItems;
		
		uint64_t m_selectedInstruction 	= 0;
		uint64_t m_lastEIPInstruction	= 0;
		bool m_needUpdateScroll 		= false;
		
		float m_width 				= 400;
		float m_height 				= 400;
		
		// We keep a copy of our fonts so that we can show a different font than the main window
		ImFont *m_defaultFont 		= 0;
		ImFont *m_boldFont 			= 0;
		ImFont *m_italicFont 		= 0;
		ImFont *m_boldItalicFont 	= 0;
		
};

#endif
