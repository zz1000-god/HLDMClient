/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// menu.cpp
//
// generic menu handler
//
#define NOMINMAX 
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include <keydefs.h>

#include "vgui_TeamFortressViewport.h"
#include <string_view>

#define MAX_MENU_STRING	512
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

int KB_ConvertString(char* in, char** ppout);

DECLARE_MESSAGE(m_Menu, ShowMenu);

int CHudMenu::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(ShowMenu);

	// Register console variables
	hud_menu_fkeys = CVAR_CREATE("hud_menu_fkeys", "0", FCVAR_ARCHIVE);
	hud_menu_fkeys_cooldown = CVAR_CREATE("hud_menu_fkeys_cooldown", "1.5", FCVAR_ARCHIVE);

	InitHUDData();

	return 1;
}

void CHudMenu::InitHUDData(void)
{
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	m_iFlags &= ~HUD_ACTIVE;
	m_flMenuCloseTime = -1;
	Reset();
}

void CHudMenu::Reset(void)
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = FALSE;
}

int CHudMenu::VidInit(void)
{
	return 1;
}

/*=================================
  ParseEscapeToken

  Interprets the given escape token (backslash followed by a letter). The
  first character of the token must be a backslash.  The second character
  specifies the operation to perform:

   \w : White text (this is the default)
   \d : Dim (gray) text
   \y : Yellow text
   \r : Red text
   \R : Right-align (just for the remainder of the current line)
=================================*/

static int menu_r, menu_g, menu_b, menu_x, menu_ralign;

static inline const char* ParseEscapeToken(const char* token)
{
	if (*token != '\\')
		return token;

	token++;

	switch (*token)
	{
	case '\0':
		return token;

	case 'w':
		menu_r = 255;
		menu_g = 255;
		menu_b = 255;
		break;

	case 'd':
		menu_r = 100;
		menu_g = 100;
		menu_b = 100;
		break;

	case 'y':
		menu_r = 255;
		menu_g = 210;
		menu_b = 64;
		break;

	case 'r':
		menu_r = 210;
		menu_g = 24;
		menu_b = 0;
		break;

	case 'R':
		menu_x = ScreenWidth / 2;
		menu_ralign = TRUE;
		break;
	}

	return ++token;
}
// Get optimal Y position for menu start
int CHudMenu::GetStartY(int lineCount, int lineHeight)
{
	int height = (lineCount + 1) * lineHeight; // +1 to account for the last line missing a \n
	int top = (ScreenHeight / 2) - (height / 2); // Centered vertically
	int bottom = top + height;

	// Make sure menu doesn't occlude other HUD elements (adapt based on your HUD layout)
	// This is a simplified version - you might need to check for chat position, etc.
	int minTop = 50; // Minimum distance from top
	int maxBottom = ScreenHeight - 100; // Maximum distance from bottom

	if (top < minTop)
	{
		top = minTop;
	}
	else if (bottom > maxBottom)
	{
		top = maxBottom - height;
		if (top < minTop)
			top = minTop;
	}

	return top;
}

int CHudMenu::Draw(float flTime)
{
	// check for if menu is set to disappear
	if (m_flShutoffTime > 0)
	{
		if (m_flShutoffTime <= gHUD.m_flTime)
		{  // times up, shutoff
			CloseMenu();
			return 1;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	if (gViewPort && gViewPort->IsScoreBoardVisible())
		return 1;

	SCREENINFO screenInfo;
	screenInfo.iSize = sizeof(SCREENINFO);
	gEngfuncs.pfnGetScreenInfo(&screenInfo);

	// draw the menu, along the left-hand side of the screen
	const int lineHeight = max(12, screenInfo.iCharHeight);

	// count the number of newlines
	int nlc = 0;
	int i;
	for (i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++)
	{
		if (g_szMenuString[i] == '\n')
			nlc++;
	}

	int y = GetStartY(nlc, lineHeight);

	menu_r = 255;
	menu_g = 255;
	menu_b = 255;
	menu_x = 20;
	menu_ralign = FALSE;

	const char* sptr = g_szMenuString;

	while (*sptr != '\0')
	{
		if (*sptr == '\\')
		{
			sptr = ParseEscapeToken(sptr);
		}
		else if (*sptr == '\n')
		{
			menu_ralign = FALSE;
			menu_x = 20;
			y += lineHeight;

			sptr++;
		}
		else
		{
			char menubuf[80];
			const char* ptr = sptr;
			while (*sptr != '\0' && *sptr != '\n' && *sptr != '\\')
			{
				sptr++;
			}
			strncpy(menubuf, ptr, min((sptr - ptr), (int)sizeof(menubuf)));
			menubuf[min((sptr - ptr), (int)(sizeof(menubuf) - 1))] = '\0';

			// Enhanced F-key display logic
			if (hud_menu_fkeys->value && !menu_ralign)
			{
				// Prepend menu items with 'F'
				// Only check first 16 chars to reduce false number detection
				std::string_view menubufView(menubuf, std::min(strlen(menubuf), size_t(16)));
				size_t firstNonSpace = menubufView.find_first_not_of(" \t");
				if (firstNonSpace != std::string_view::npos && firstNonSpace <= sizeof(menubuf) - 2)
				{
					// First char is a digit and next one isn't
					if (menubuf[firstNonSpace] >= '0' && menubuf[firstNonSpace] <= '9' &&
						!(menubuf[firstNonSpace + 1] >= '0' && menubuf[firstNonSpace + 1] <= '9'))
					{
						int digit = menubuf[firstNonSpace] - '0';
						int shift = digit == 0 ? 2 : 1;

						// Shift the string by 1/2 chars
						memmove(menubuf + firstNonSpace + shift, menubuf + firstNonSpace, sizeof(menubuf) - firstNonSpace - shift);

						// Set the char
						if (digit == 0)
						{
							menubuf[firstNonSpace] = 'F';
							menubuf[firstNonSpace + 1] = '1';
						}
						else
						{
							menubuf[firstNonSpace] = 'F';
						}
					}
				}
			}
			if (menu_ralign)
			{
				// IMPORTANT: Right-to-left rendered text does not parse escape tokens!
				menu_x = gHUD.DrawHudStringReverse(menu_x, y, 0, menubuf, menu_r, menu_g, menu_b);
			}
			else
			{
				menu_x = gHUD.DrawHudString(menu_x, y, 320, menubuf, menu_r, menu_g, menu_b);
			}
		}
	}

	return 1;
}
// Enhanced weapon slot selection with F-key support
bool CHudMenu::OnWeaponSlotSelected(int slotIdx)
{
	if (!m_fMenuDisplayed)
		return false;

	// Якщо fkeys увімкнено — не дозволяємо вибір 1–0
	if (hud_menu_fkeys->value)
		return false;

	SelectMenuItem(slotIdx + 1); // slotIdx — 0..9
	return true;
}

bool CHudMenu::OnKeyPressed(int keynum)
{
	if (!m_fMenuDisplayed)
		return false;

	if (!hud_menu_fkeys->value)
		return false;
	
	if (!(keynum >= K_F1 && keynum <= K_F10))
		return false;

	if (m_fMenuDisplayed) {
		SelectMenuItem(keynum - K_F1 + 1);
		return true;
	}
	else if (m_flMenuCloseTime != -1 && gEngfuncs.GetAbsoluteTime() - m_flMenuCloseTime < hud_menu_fkeys_cooldown->value) {
		return true;
	}

	return true;
}

// selects an item from the menu
void CHudMenu::SelectMenuItem(int menu_item)
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ((menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item - 1))))
	{
		char szbuf[32];
		sprintf(szbuf, "menuselect %d\n", menu_item);
		EngineClientCmd(szbuf);

		// remove the menu
		CloseMenu();
	}
}

// Enhanced menu closing
void CHudMenu::CloseMenu(void)
{
	m_fMenuDisplayed = 0;
	m_iFlags &= ~HUD_ACTIVE;
	m_flMenuCloseTime = gEngfuncs.GetClientTime();
}

// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu::MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf)
{
	char* temp = NULL;

	BEGIN_READ(pbuf, iSize);

	m_bitsValidSlots = READ_SHORT();
	int DisplayTime = READ_CHAR();
	int NeedMore = READ_BYTE();

	if (DisplayTime > 0)
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if (m_bitsValidSlots)
	{
		if (!m_fWaitingForMore) // this is the start of a new menu
		{
			strncpy(g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING);
		}
		else
		{  // append to the current menu string
			strncat(g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString));
		}
		g_szPrelocalisedMenuString[MAX_MENU_STRING - 1] = 0;  // ensure null termination (strncat/strncpy does not)

		if (!NeedMore)
		{  // we have the whole string, so we can localise it now
			strncpy(g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString(g_szPrelocalisedMenuString), MAX_MENU_STRING);
			g_szMenuString[MAX_MENU_STRING - 1] = '\0';

			// Swap in characters
			if (KB_ConvertString(g_szMenuString, &temp))
			{
				strncpy(g_szMenuString, temp, MAX_MENU_STRING);
				g_szMenuString[MAX_MENU_STRING - 1] = '\0';
				free(temp);
			}
		}

		m_fMenuDisplayed = 1;
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		// no valid slots means that the menu should be turned off
		CloseMenu();
	}

	m_fWaitingForMore = NeedMore;

	return 1;
}