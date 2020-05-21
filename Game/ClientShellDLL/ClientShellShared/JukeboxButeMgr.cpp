#include "stdafx.h"
#include "JukeboxButeMgr.h"

CJukeboxButeMgr* g_pJukeboxButeMgr = nullptr;

// Model definition...kinda
#define JBM_THEME "Theme"
#define JBM_THEME_NAME "Name"
#define JBM_THEME_DIRECTORY "Directory"
#define JBM_THEME_CONTROL_FILE "ControlFile"
#define JBM_THEME_REQUIRES_GOTY "RequiresGOTY"

#define JBM_SONG "Song"
#define JBM_SONG_NAME "Name"
#define JBM_SONG_INTENSITY "IntensityLevel"
#define JBM_SONG_THEME "Theme"


CJukeboxButeMgr::CJukeboxButeMgr()
{
}

CJukeboxButeMgr::~CJukeboxButeMgr()
{
	Term();
}

LTBOOL CJukeboxButeMgr::Init(const char* szAttributeFile)
{
	if (g_pJukeboxButeMgr || !szAttributeFile) return LTFALSE;
	if (!Parse(szAttributeFile)) return LTFALSE;

	m_nThemeIDCount = 0;
	m_nSongIDCount = 0;

	// Set up global pointer

	g_pJukeboxButeMgr = this;

	// This is nicer than c-string-ing.
	std::string sTagName = "";

	// See how many attribute templates there are

	m_nThemeIDCount = 0;
	sTagName = JBM_THEME + std::to_string(m_nThemeIDCount);
	while (m_buteMgr.Exist(sTagName.c_str()))
	{
		m_nThemeIDCount++;
		sTagName = JBM_THEME + std::to_string(m_nThemeIDCount);
	}

	m_nSongIDCount = 0;
	sTagName = JBM_SONG + std::to_string(m_nSongIDCount);
	while (m_buteMgr.Exist(sTagName.c_str()))
	{
		m_nSongIDCount++;
		sTagName = JBM_SONG + std::to_string(m_nSongIDCount);
	}

	return LTTRUE;
}

void CJukeboxButeMgr::Term()
{
	g_pJukeboxButeMgr = nullptr;
}

//
// Theme stuff
//

void CJukeboxButeMgr::GetThemeName(int nThemeID, char* pBuf, uint16 nBufLen)
{
	pBuf[0] = LTNULL;

	if (nThemeID < 0 || nThemeID > m_nThemeIDCount) return;

	std::string sTagName = JBM_THEME + std::to_string(nThemeID);

	m_buteMgr.GetString(sTagName.c_str(), JBM_THEME_NAME, "", pBuf, nBufLen);
}

void CJukeboxButeMgr::GetThemeDirectory(int nThemeID, char* pBuf, uint16 nBufLen)
{
	pBuf[0] = LTNULL;

	if (nThemeID < 0 || nThemeID > m_nThemeIDCount) return;

	std::string sTagName = JBM_THEME + std::to_string(nThemeID);

	m_buteMgr.GetString(sTagName.c_str(), JBM_THEME_DIRECTORY, "", pBuf, nBufLen);
}

void CJukeboxButeMgr::GetThemeControlFile(int nThemeID, char* pBuf, uint16 nBufLen)
{
	pBuf[0] = LTNULL;

	if (nThemeID < 0 || nThemeID > m_nThemeIDCount) return;

	std::string sTagName = JBM_THEME + std::to_string(nThemeID);

	m_buteMgr.GetString(sTagName.c_str(), JBM_THEME_CONTROL_FILE, "", pBuf, nBufLen);
}

bool CJukeboxButeMgr::GetThemeRequiresGOTY(int nThemeID)
{
	if (nThemeID < 0 || nThemeID > m_nThemeIDCount) return false;

	std::string sTagName = JBM_THEME + std::to_string(nThemeID);

	return m_buteMgr.GetBool(sTagName.c_str(), JBM_THEME_REQUIRES_GOTY);
}

//
// Song stuff
//

void CJukeboxButeMgr::GetSongName(int nSongID, char* pBuf, uint16 nBufLen)
{
	pBuf[0] = LTNULL;

	if (nSongID < 0 || nSongID > m_nSongIDCount) return;

	std::string sTagName = JBM_SONG + std::to_string(nSongID);
	m_buteMgr.GetString(sTagName.c_str(), JBM_SONG_NAME, "", pBuf, nBufLen);
}

int CJukeboxButeMgr::GetSongIntensityLevel(int nSongID)
{
	if (nSongID < 0 || nSongID > m_nSongIDCount) return 0; // 0 is usually silence!

	std::string sTagName = JBM_SONG + std::to_string(nSongID);

	return m_buteMgr.GetInt(sTagName.c_str(), JBM_SONG_INTENSITY);
}

int CJukeboxButeMgr::GetSongThemeID(int nSongID)
{
	if (nSongID < 0 || nSongID > m_nSongIDCount) return 0;

	std::string sTagName = JBM_SONG + std::to_string(nSongID);

	return m_buteMgr.GetInt(sTagName.c_str(), JBM_SONG_THEME);
}
