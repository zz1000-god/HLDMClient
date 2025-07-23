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

bool AppendPlayerIfOnlyColorTags(char* text, size_t maxLen)
{
	char* original = text;
	char lastColorTag[3] = "";
	bool onlyTags = true;

	while (*text)
	{
		unsigned char c = (unsigned char)*text;

		if (c == '^' && isdigit((unsigned char)*(text + 1)) && *(text + 1) != '0')
		{
			lastColorTag[0] = *text;
			lastColorTag[1] = *(text + 1);
			lastColorTag[2] = '\0';
			text += 2;
		}
		else if (c <= 32)
		{
			++text;
		}
		else
		{
			onlyTags = false;
			break;
		}
	}

	if (onlyTags)
	{
		strncat(original, lastColorTag, maxLen - strlen(original) - 1);
		strncat(original, "Player", maxLen - strlen(original) - 1);
		return true;
	}

	return false;
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

								AppendPlayerIfOnlyColorTags(szRepString, MAX_PLAYER_NAME_LENGTH);

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

	// Draw the status bar lines
	for (int i = 0; i < MAX_STATUSBAR_LINES; i++)
	{
		char* text = m_szStatusBar[i];

		char plainText[MAX_STATUSTEXT_LENGTH] = { 0 };
		int TextHeight = 0, TextWidth = 0;

		color_tags::strip_color_tags(plainText, text, sizeof(plainText));

		GetConsoleStringSize(plainText, &TextWidth, &TextHeight);

		int x = 8;
		int y = Y_START - (4 + TextHeight * i); // draw along bottom of screen

		// let user set status ID bar centering
		if ((i == STATUSBAR_ID_LINE) && CVAR_GET_FLOAT("hud_centerid"))
		{
			x = max(0, max(2, (ScreenWidth - TextWidth)) / 2);
			y = (ScreenHeight / 2) + (TextHeight * CVAR_GET_FLOAT("hud_centerid"));
		}

		if (text)
			gHUD.DrawConsoleStringWithColorTags(
				x,
				y,
				text,
				true,
				m_pflNameColors[i][0],
				m_pflNameColors[i][1],
				m_pflNameColors[i][2]
			);
		else
			DrawConsoleString(x, y, text);
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
