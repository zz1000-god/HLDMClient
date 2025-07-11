// statusbar.cpp з п≥дтримкою кольорових тег≥в ^1Ц^9 та центрованим вир≥внюванн€м

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_StatusBar, StatusText);
DECLARE_MESSAGE(m_StatusBar, StatusValue);

#ifdef _TFC
#define STATUSBAR_ID_LINE		2
#else
#define STATUSBAR_ID_LINE		1
#endif

float* GetClientColor(int clientIndex);
extern float g_ColorYellow[3];

int CHudStatusBar::Init(void)
{
	gHUD.AddHudElem(this);
	HOOK_MESSAGE(StatusText);
	HOOK_MESSAGE(StatusValue);
	Reset();
	CVAR_CREATE("hud_centerid", "0", FCVAR_ARCHIVE);
	return 1;
}

int CHudStatusBar::VidInit(void)
{
	return 1;
}

void CHudStatusBar::Reset(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	memset(m_szStatusText, 0, sizeof m_szStatusText);
	memset(m_szStatusBar, 0, sizeof m_szStatusBar);
	memset(m_iStatusValues, 0, sizeof m_iStatusValues);
	m_iStatusValues[0] = 1;
	for (int i = 0; i < MAX_STATUSBAR_LINES; i++)
		m_pflNameColors[i] = g_ColorYellow;
}

void CHudStatusBar::ParseStatusString(int line_num)
{
	char szBuffer[MAX_STATUSTEXT_LENGTH] = { 0 };
	gHUD.m_TextMessage.LocaliseTextString(m_szStatusText[line_num], szBuffer, MAX_STATUSTEXT_LENGTH);

	char* src = szBuffer;
	char* dst = m_szStatusBar[line_num];
	char* dst_start = dst;

	while (*src != 0)
	{
		while (*src == '\n') src++;
		if ((src - szBuffer) >= MAX_STATUSTEXT_LENGTH || (dst - dst_start) >= MAX_STATUSTEXT_LENGTH)
			break;

		int index = atoi(src);
		if ((index >= 0 && index < MAX_STATUSBAR_VALUES) && (m_iStatusValues[index] != 0))
		{
			while (*src >= '0' && *src <= '9') src++;
			if (*src == '\n' || *src == 0) continue;

			while (*src != '\n' && *src != 0)
			{
				if (*src != '%')
					*dst++ = *src++;
				else
				{
					char valtype = *(++src);
					if (valtype == '%') { *dst++ = valtype; src++; continue; }

					index = atoi(++src);
					while (*src >= '0' && *src <= '9') src++;

					if (index >= 0 && index < MAX_STATUSBAR_VALUES)
					{
						int indexval = m_iStatusValues[index];
						char szRepString[MAX_PLAYER_NAME_LENGTH];
						szRepString[0] = 0;

						switch (valtype)
						{
						case 'p':
							gEngfuncs.pfnGetPlayerInfo(indexval, &g_PlayerInfoList[indexval]);
							if (g_PlayerInfoList[indexval].name)
							{
								strncpy(szRepString, g_PlayerInfoList[indexval].name, MAX_PLAYER_NAME_LENGTH);
								m_pflNameColors[line_num] = GetClientColor(indexval);
							}
							else strcpy(szRepString, "******");
							break;
						case 'i':
							sprintf(szRepString, "%d", indexval);
							break;
						}

						for (char* cp = szRepString; *cp && ((dst - dst_start) < MAX_STATUSTEXT_LENGTH); cp++, dst++)
							*dst = *cp;
					}
				}
			}
		}
		else { while (*src && *src != '\n') src++; }
	}
	*dst = '\0';
}

int CHudStatusBar::Draw(float fTime)
{
	if (m_bReparseString)
	{
		for (int i = 0; i < MAX_STATUSBAR_LINES; i++)
		{
			m_pflNameColors[i] = g_ColorYellow;
			ParseStatusString(i);
		}
		m_bReparseString = FALSE;
	}

	int Y_START = ScreenHeight - 52;

	for (int i = 0; i < MAX_STATUSBAR_LINES; i++)
	{
		int baseX = 8;
		int y = Y_START - (4 + 13 * i);
		char* text = m_szStatusBar[i];
		if (!text[0]) continue;

		int totalWidth = 0, textHeight = 13;
		color_tags::for_each_colored_substr(text, [&](const char* str, bool, int, int, int) {
			int w, h;
			GetConsoleStringSize((char*)str, &w, &h);
			totalWidth += w;
			textHeight = h;
			});

		if ((i == STATUSBAR_ID_LINE) && CVAR_GET_FLOAT("hud_centerid")) {
			baseX = max(0, max(2, (ScreenWidth - totalWidth)) / 2);
			y = (ScreenHeight / 2) + (textHeight * CVAR_GET_FLOAT("hud_centerid"));
		}

		int x = baseX;
		color_tags::for_each_colored_substr(text, [=, &x](const char* str, bool customColor, int r, int g, int b) mutable {
			if (customColor)
				gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
			else
				gEngfuncs.pfnDrawSetTextColor(g_ColorYellow[0], g_ColorYellow[1], g_ColorYellow[2]);

			DrawConsoleString(x, y, (char*)str);
			int w, h;
			GetConsoleStringSize((char*)str, &w, &h);
			x += w;
			});
	}

	return 1;
}

int CHudStatusBar::MsgFunc_StatusText(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int line = READ_BYTE();
	if (line < 0 || line > MAX_STATUSBAR_LINES) return 1;
	strncpy(m_szStatusText[line], READ_STRING(), MAX_STATUSTEXT_LENGTH);
	m_szStatusText[line][MAX_STATUSTEXT_LENGTH - 1] = 0;
	m_iFlags |= HUD_ACTIVE;
	m_bReparseString = TRUE;
	return 1;
}

int CHudStatusBar::MsgFunc_StatusValue(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int index = READ_BYTE();
	if (index < 1 || index > MAX_STATUSBAR_VALUES) return 1;
	m_iStatusValues[index] = READ_SHORT();
	m_bReparseString = TRUE;
	return 1;
}
