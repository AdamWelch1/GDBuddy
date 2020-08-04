#include "gui_codeview.h"
// #include "../gdbmi/gdbmi.h"

extern GDBMI *gdb;
// extern GuiManager *gui;

void GuiCodeView::draw()
{
	if(m_sameLine)
		SameLine();
	ImVec2 mwSize = m_parent->getMainWindowSize();
	
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
	PushStyleColor(ImGuiCol_ChildBg, m_parent->getColor(GuiItem::CodeViewColumnHeader));
	
	if(BeginChild("CodeViewPane", { mwSize.x * m_width, mwSize.y * m_height }, true))
	{
		mutex &m_codeLinesMutex = m_parent->getCodeLinesMtx();
		AsmDump &m_codeLines = m_parent->getCodeLines();
		GDBMI::StepFrame stepFrame = gdb->getStepFrame();
		GDBMI::CurrentInstruction curPos = gdb->getCurrentExecutionPos();
		
		m_codeLinesMutex.lock();
		if(m_codeLines.size() > 0)
		{
			ImFont *tmpFont = GetFont();
			ImVec2 strSize = tmpFont->CalcTextSizeA(APP_FONT_SIZE, FLT_MAX, FLT_MAX, m_codeLines[0].addr.c_str());
			
			float winWidth = GetWindowWidth();
			float adj = (10.0 - strSize.x + 3.0) / winWidth;
			PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(adj, 0));
			
			// This adds some space above the column headers
			{
				PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0, -10.0));
				Columns(3);
				SetColumnWidth(-1, 150.0);
				SetColumnWidth(1, 200.0);
				
				NextColumn();
				Text(" ");
				NextColumn();
				Text(" ");
				NextColumn();
				Text(" ");
				
				PopStyleVar(1);
			}
			
			NextColumn();
			PushFont(m_boldFont);
			Text("Address");
			NextColumn();
			Text("Instruction");
			PopFont();
			NextColumn();
			PopStyleColor(1);
			Columns(1);
			
			
			PushStyleVar(ImGuiStyleVar_ChildBorderSize, (float) 0.0);
			
			if(BeginChild("DisassemblyInstructionList", ImVec2(0, 0), false, 0))
			{
				uint32_t lineCtr = 0;
				Columns(3);
				Separator();
				SetColumnWidth(-1, 150.0);
				SetColumnWidth(1, 200.0);
				// for(auto &disLine : disasLines)
				for(auto &disLine : m_codeLines)
				{
					NextColumn();
					bool selItem = (m_selectedInstruction == strtoull(disLine.addr.c_str(), 0, 16));
					// Text(disLine.addr.c_str());
					
					bool instIsPC = false;
					if(stepFrame.isValid)
					{
						// Is this instruction pointed to by the frame GDB just gave us?
						if(stepFrame.address == disLine.addr)
							instIsPC = true;
					}
					else
					{
						// Is this instruction pointed to by the program counter, $pc (ex. eip, rip)?
						if(curPos.first == strtoull(disLine.addr.c_str(), 0, 16))
							instIsPC = true;
					}
					
					if(instIsPC)
						PushFont(m_boldFont);
						
					PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewAddress));
					Selectable(disLine.addr.c_str(),
							   (instIsPC ? (bool) true : selItem),
							   ImGuiSelectableFlags_SpanAllColumns);
					PopStyleColor(1);
					
					if(instIsPC)
						SetScrollHereY(0.5);
						
					if(IsItemClicked())
						m_selectedInstruction = strtoull(disLine.addr.c_str(), 0, 16);
						
					NextColumn();
					
					if(instIsPC)
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewInstructionActive));
					else
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewInstruction));
						
					Text("%s", disLine.instr.c_str());
					
					PopStyleColor(1);
					NextColumn();
					
					if(instIsPC)
						PopFont();
				}
			}
			EndChild();
			
			PopStyleVar(2);
		}
		else
			PopStyleColor(1);
			
		m_codeLinesMutex.unlock();
	}
	else
		PopStyleColor(1);
		
	EndChild();
	
	PopStyleVar(1);
}
