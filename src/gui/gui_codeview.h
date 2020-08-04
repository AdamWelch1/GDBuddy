#ifndef UNIQUE_GUI_CODEVIEW_H
#define UNIQUE_GUI_CODEVIEW_H

#include "gui_defs.h"
#include "gui_child.h"


class GuiCodeView : public GuiChild
{
	public:
	
	
		GuiCodeView();
		GuiCodeView(float w, float h)
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
	
		uint64_t m_selectedInstruction = 0;
		
		float m_width 				= 400;
		float m_height 				= 400;
		
		// We keep a copy of our fonts so that we can show a different font than the main window
		ImFont *m_defaultFont 		= 0;
		ImFont *m_boldFont 			= 0;
		ImFont *m_italicFont 		= 0;
		ImFont *m_boldItalicFont 	= 0;
		
};

#endif
