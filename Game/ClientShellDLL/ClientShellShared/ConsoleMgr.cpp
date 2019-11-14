#include "stdafx.h"
#include "ConsoleMgr.h"

#include "LTGUIMgr.h"
#include "LTPoly.h"
#include "InterfaceResMgr.h"

ConsoleMgr* g_pConsoleMgr = NULL;
extern CInterfaceResMgr* g_pInterfaceResMgr;

void ShowHelpListCommand(int argc, char** argv)
{
	if (!g_pConsoleMgr) {
		return;
	}

	g_pLTClient->CPrint("Available game commands:");
	for (auto item : g_pConsoleMgr->GetHelpList()) {
		g_pLTClient->CPrint(item.c_str());
	}
}

ConsoleMgr::ConsoleMgr()
{
	g_pConsoleMgr = this;

	m_bInitialized = false;
	m_bVisible = false;
	m_hConsoleSurface = NULL;
	memset(m_szEdit, 0, sizeof(m_szEdit));

	m_iWidth = 320;			// px
	m_iHeight = 240;		// Pixels
	m_iFontSize = 14;		// Pixels?
	m_iLineSpacing = 16;	// Pixels!

	// Position in History
	m_iCurrentPosition = 0;
	// Position in UI elements
	m_iCursorPosition = 0;

	g_pLTClient->RegisterConsoleProgram("Help", ShowHelpListCommand);
}

ConsoleMgr::~ConsoleMgr()
{
	g_pLTClient->UnregisterConsoleProgram("Help");

	Destroy();

	if (g_pConsoleMgr) {
		g_pConsoleMgr = NULL;
	}

	if (m_hConsoleSurface) {
		g_pInterfaceResMgr->FreeSharedSurface(m_hConsoleSurface);
		m_hConsoleSurface = NULL;
	}
}

void ConsoleMgr::Init()
{
	// If we're re-initializing, then destroy all the items
	if (m_bInitialized) {
		Destroy();
	}

	RMode currentMode;
	g_pLTClient->GetRenderMode(&currentMode);

	m_iHeight = (int)((float)currentMode.m_Height * 0.50f);
	m_iWidth = (int)currentMode.m_Width;

	// Calculate the amount of items we need to fill the space, and then minus one for our edit line.
	int iLineItemLength = (int)((float)m_iHeight / (float)m_iLineSpacing) - 1;

	m_Window.Create(g_pInterfaceResMgr->GetTexture("interface\\console.dtx"), m_iWidth, m_iHeight);

	// Create the lines for our console text
	// TODO: Maybe look into large text fields? Not sure if this will cause any performance concerns.
	for (int i = 0; i < iLineItemLength; i++) {

		CLTGUITextCtrl* pText = debug_new(CLTGUITextCtrl);
		if (!pText->Create("", LTNULL, LTNULL, g_pInterfaceResMgr->GetFont(4), m_iFontSize, NULL))
		{
			debug_delete(pText);
			pText = LTNULL;
		}

		m_pLineItems.push_back(pText);
		m_Window.AddControl(pText, { 0, 0 });
	}
	
	// This is basically the little symbol, in our case it's `>` in front of the edit line
	m_pEditText = debug_new(CLTGUITextCtrl);
	if (!m_pEditText->Create("", LTNULL, LTNULL, g_pInterfaceResMgr->GetFont(4), m_iFontSize, NULL))
	{
		debug_delete(m_pEditText);
		m_pEditText = LTNULL;
	}

	m_pEditText->SetString(">");
	m_pEditText->SetColors(argbWhite, argbWhite, argbWhite);
	m_Window.AddControl(m_pEditText, { 0,0 });
	
	m_pEdit = debug_new(CLTGUIEditCtrl);
	if (!m_pEdit->Create(g_pLTClient, 1, g_pInterfaceResMgr->GetFont(4), m_iFontSize, 256, NULL, m_szEdit))
	{
		debug_delete(m_pEdit);
		m_pEdit = LTNULL;
	}

	m_pEdit->Show(LTTRUE);
	m_pEdit->Enable(LTTRUE);
	m_pEdit->EnableCaret(LTTRUE);
	m_pEdit->SetColors(argbWhite, argbWhite, argbWhite);
	
	// Add the edit control, and then set it as the selected control
	m_Window.AddControl(m_pEdit, { 0, 0 });
	m_Window.SetSelection(iLineItemLength + 1);

	m_bInitialized = true;
}

void ConsoleMgr::Destroy()
{
	if (!m_bInitialized) {
		return;
	}

	m_pEdit->Destroy();
	m_pEditText->Destroy();

	for (auto item : m_pLineItems) {
		item->Destroy();
	}

	m_Window.Destroy();

	m_pLineItems.clear();
}

LTBOOL ConsoleMgr::HandleChar(unsigned char c)
{
	if (!m_bVisible) return LTFALSE;

	// Ignore console key
	if (c == VK_OEM_3) {
		return LTFALSE;
	}

	return m_pEdit->HandleChar(c);
}


LTBOOL ConsoleMgr::HandleKeyDown(int key, int rep)
{
	if (!m_bVisible) {
		if (key == VK_OEM_3) {
			Show(true);
			return LTTRUE;
		}

		return LTFALSE;
	}

	switch (key) {
	case VK_OEM_3:
	case VK_ESCAPE:
	{
		Show(false);
		return LTTRUE;
	} break;

	case VK_RETURN:
	{
		Send();
		return LTTRUE;
	} break;
	case VK_UP:
		MoveUp(false);
		break;
	case VK_DOWN:
		MoveDown(false);
		break;
	case VK_PRIOR: // Page Up
		MoveUp(true);
		break;
	case VK_NEXT: // Page Down
		MoveDown(true);
		break;
	}

	m_pEdit->HandleKeyDown(key, rep);
	return LTTRUE;
}

void ConsoleMgr::Read(CConsolePrintData* pData)
{
	HistoryData data;

	data.iColour = SET_ARGB(255, pData->m_Color.r, pData->m_Color.g, pData->m_Color.b);
	data.sMessage = pData->m_pMessage;
	data.iLevel = pData->m_nFilterLevel;
	
	m_History.push_back(data);

	// Ok we can't actually adjust our position if we're not init'd.
	// This happens when the game is launching and setting stuff up before consolemgr is init'd.
	if (!m_bInitialized) {
		m_iCurrentPosition++;
		return;
	}

	MoveDown(false);
}

void ConsoleMgr::Send()
{
	MoveDown(true);

	// Echo it back
	g_pLTClient->CPrint(m_szEdit);

	// Send it off!
	g_pLTClient->RunConsoleString(m_szEdit);

	// Clear our command string
	m_pEdit->SetText("");
	memset(m_szEdit, 0, sizeof(m_szEdit));
}

void ConsoleMgr::Draw()
{
	if (!m_bVisible) {
		return;
	}

	m_pEdit->UpdateData(LTTRUE);

	LTIntPt pos = { 8, 0 };

	m_Window.SetScale(1.0f);

	for (auto item : m_pLineItems) {
		item->SetString("");
	}

	auto selectionColour = SET_ARGB(255, 255, 128, 255);

	int index = 0;
	for (auto item : m_HistorySlice) {
		if (index >= m_pLineItems.size()) {
			break;
		}

		CLTGUITextCtrl* pText = m_pLineItems.at(index);

		pText->SetBasePos(pos);
		pText->SetString(item.sMessage.c_str());
		pText->SetColors(selectionColour, item.iColour, item.iColour);

		index++;
		pos.y += m_iLineSpacing;
	}

	// Make sure our edit input is always at the bottom!
	pos.y = m_iLineSpacing * m_pLineItems.size();

	m_pEditText->SetBasePos(pos);
	pos.x += m_iLineSpacing / 2;
	m_pEdit->SetBasePos(pos);

	m_Window.Render();
}

void ConsoleMgr::Show(bool bShow)
{
	// For now let's disable it in multiplayer!
	if (IsMultiplayerGame()) {
		m_bVisible = false;
	}

	// Pause the game
	g_pGameClientShell->PauseGame(bShow, LTTRUE);

	// Clear our command string
	m_pEdit->SetText("");
	memset(m_szEdit, 0, sizeof(m_szEdit));

	MoveDown(true);

	m_bVisible = bShow;
}

void ConsoleMgr::MoveUp(bool bTop)
{
	if (bTop) {
#ifdef CONSOLE_CURSOR
		m_iCurrentPosition = 0;
#else
		m_iCurrentPosition = m_pLineItems.size();
#endif
		m_iCursorPosition = 0;
	}
	else {
#ifdef CONSOLE_CURSOR
		if (m_iCurrentPosition > 0) {
#else
		if (m_iCurrentPosition > m_pLineItems.size()) {
#endif
			m_iCurrentPosition--;
		}
		if (m_iCursorPosition > 0) {
			m_iCursorPosition--;
		}
	}

	AdjustView();
}

void ConsoleMgr::MoveDown(bool bBottom)
{
	if (bBottom) {
		m_iCurrentPosition = m_History.size();
		m_iCursorPosition = m_pLineItems.size() - 1;
	}
	else {
		if (m_iCurrentPosition < m_History.size()) {
			m_iCurrentPosition++;
		}
		if (m_iCursorPosition < m_pLineItems.size() - 1) {
			m_iCursorPosition++;
		}
	}

	AdjustView();
}

void ConsoleMgr::AdjustView()
{
	m_HistorySlice.clear();

	int iBegin = m_iCurrentPosition - m_pLineItems.size();
	int iEnd = m_iCurrentPosition + m_pLineItems.size();

	// Clamp our being/end to -> 0 - Max Size
	if (iBegin < 0) {
		iBegin = 0;
	}
	if (iEnd > m_History.size()) {
		iEnd = m_History.size();
	}

	// Special case:
	// If we haven't filled the screen with commands, don't allow them to move up!
	if (m_History.size() < m_pLineItems.size()) {
		iBegin = 0;
	}

	// Remake the vector with the new slice
	std::vector<HistoryData> historySlice(m_History.begin() + iBegin, m_History.begin() + iEnd);

	for (auto item : historySlice) {
		m_HistorySlice.push_back(item);
	}

	historySlice.clear();

#ifdef CONSOLE_CURSOR
	m_Window.SetSelection(m_iCursorPosition);
#endif
}

void ConsoleMgr::AddToHelp(std::string command)
{
	m_HelpList.push_back(command);
}

void ConsoleMgr::RemoveFromHelp(std::string command)
{
	int index = 0;
	for (auto item : m_HelpList) {
		if (command.compare(item) == 0) {
			item.erase(item.begin() + index);
		}
		index++;
	}
}
