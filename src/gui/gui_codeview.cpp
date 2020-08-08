#include "gui_codeview.h"
// #include "../gdbmi/gdbmi.h"

extern GDBMI *gdb;
// extern GuiManager *gui;

GuiCodeView::GuiCodeView()
{
	GuiMenuItem mi;
	mi.label = "Set breakpoint";
	mi.shortcut = "Space";
	mi.cbFunc = [](GuiMenuItem * mItem)
	{
		GMI_Data *bpInfo = (GMI_Data *)(mItem->rawStruct);
		
		if(bpInfo->bpIsSet)
			gdb->deleteBreakpoint(bpInfo->bpNum);
		else
			gdb->insertBreakpointAtAddress(bpInfo->setAddr);
	};
	
	m_menuItems.push_back(mi);
}

void GuiCodeView::draw()
{
	if(m_sameLine)
		SameLine();
	ImVec2 mwSize = m_parent->getMainWindowSize();
	
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
	PushStyleColor(ImGuiCol_ChildBg, m_parent->getColor(GuiItem::CodeViewColumnHeader));
	
	if(BeginChild("CodeViewPane", { mwSize.x * m_width, mwSize.y * m_height }, true))
	{
		vector<GDBMI::BreakpointInfo> bpListCopy;
		m_parent->getBreakpointMutex().lock();
		bpListCopy = m_parent->getBreakpointList();
		m_parent->getBreakpointMutex().unlock();
		
		auto addrIsBP = [&](string address) -> const GDBMI::BreakpointInfo*
		{
			for(auto &bp : bpListCopy)
			{
				if(bp.addr == address)
					return &bp;
			}
			
			return 0;
		};
		
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
					const GDBMI::BreakpointInfo *breakPoint = addrIsBP(disLine.addr);
					
					NextColumn();
					bool selItem = (m_selectedInstruction == strtoull(disLine.addr.c_str(), 0, 16));
					// Text(disLine.addr.c_str());
					
					bool instIsPC = false;
					if(stepFrame.isValid)
					{
						// Is this instruction pointed to by the frame GDB just gave us?
						if(stepFrame.address == disLine.addr)
						{
							instIsPC = true;
							uint64_t tmpaddr = strtoull(stepFrame.address.c_str(), 0, 16);
							
							if(m_lastEIPInstruction != tmpaddr)
							{
								m_lastEIPInstruction = tmpaddr;
								m_needUpdateScroll = true;
							}
						}
					}
					else
					{
						// Is this instruction pointed to by the program counter, $pc (ex. eip, rip)?
						if(curPos.first == strtoull(disLine.addr.c_str(), 0, 16))
						{
							instIsPC = true;
							
							if(m_lastEIPInstruction != curPos.first)
							{
								m_lastEIPInstruction = curPos.first;
								m_needUpdateScroll = true;
							}
						}
					}
					
					// Set BP line background color
					if(breakPoint != 0)
						PushStyleColor(ImGuiCol_Header, m_parent->getColor(GuiItem::CodeViewBpBackground));
						
					if(instIsPC)
						PushFont(m_boldFont);
						
					// Set address text color
					if(breakPoint != 0)
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewAddressBP));
					else
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewAddress));
						
					// Draw address text
					Selectable(disLine.addr.c_str(),
							   ((instIsPC || breakPoint != 0) ? (bool) true : selItem),
							   (ImGuiSelectableFlags_SpanAllColumns |
								ImGuiSelectableFlags_AllowDoubleClick));
								
					if(breakPoint != 0)
						PopStyleColor(2);
					else
						PopStyleColor(1);
						
					// Scroll to highlighted instruction
					if(instIsPC && m_needUpdateScroll)
					{
						m_needUpdateScroll = false;
						SetScrollHereY(0.5);
					}
					
					// Highlight item if clicked
					if(IsItemClicked())
					{
						m_selectedInstruction = strtoull(disLine.addr.c_str(), 0, 16);
						
						if(IsMouseDoubleClicked(0)) // 0 = left button in ImGui...
						{
							if(breakPoint == 0)
								gdb->insertBreakpointAtAddress(disLine.addr);
							else
								gdb->deleteBreakpoint(breakPoint->number);
						}
					}
					
					if(selItem && m_parent->isKeyPressed(' '))
					{
						if(breakPoint == 0)
							gdb->insertBreakpointAtAddress(disLine.addr);
						else
							gdb->deleteBreakpoint(breakPoint->number);
							
						m_parent->clearKeyPress(' ');
					}
					
					// Show context menu on right-click and highlight item
					PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0, 8.0));
					if(BeginPopupContextItem(disLine.addr.c_str()))
					{
						m_selectedInstruction = strtoull(disLine.addr.c_str(), 0, 16);
						
						GMI_Data data;
						data.bpIsSet = (breakPoint != 0);
						data.bpNum = (breakPoint != 0) ? breakPoint->number : 0x7FFFFFF;
						strncpy(data.setAddr, disLine.addr.c_str(), 63);
						
						contextMenuHandler(data);
						EndPopup();
					}
					PopStyleVar(1);
					
					
					NextColumn();
					if(instIsPC && breakPoint != 0) // Text color if active instruction is BP
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewBPisPCText));
					else if(instIsPC) // Text color for active instruction
						PushStyleColor(ImGuiCol_Text, m_parent->getColor(GuiItem::CodeViewInstructionActive));
					else // Text color for inactive instruction
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
		{
			m_needUpdateScroll = true;
			PopStyleColor(1);
		}
		
		m_codeLinesMutex.unlock();
	}
	else
		PopStyleColor(1);
		
	EndChild();
	
	PopStyleVar(1);
}

void GuiCodeView::contextMenuHandler(GMI_Data &menuData)
{
	for(auto &mi : m_menuItems)
	{
		if(menuData.bpIsSet)
			mi.label = "Clear breakpoint";
		else
			mi.label = "Set breakpoint";
			
		memcpy(mi.rawStruct, &menuData, sizeof(menuData));
		mi.draw();
	}
}
