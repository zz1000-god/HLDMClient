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
// saytext.cpp
//
// implementation of CHudSayText class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include <malloc.h> // _alloca

#include "vgui_TeamFortressViewport.h"

extern float *GetClientColor( int clientIndex );

#define MAX_LINES	5
#define MAX_CHARS_PER_LINE	256  /* it can be less than this, depending on char size */

// allow 20 pixels on either side of the text
#define MAX_LINE_WIDTH  ( ScreenWidth - 40 )
#define LINE_START  10
static float SCROLL_SPEED = 5;

static char g_szLineBuffer[ MAX_LINES + 1 ][ MAX_CHARS_PER_LINE ];
static float *g_pflNameColors[ MAX_LINES + 1 ];
static int g_iNameLengths[ MAX_LINES + 1 ];
static float flScrollTime = 0;  // the time at which the lines next scroll up

static int Y_START = 0;
static int line_height = 0;

DECLARE_MESSAGE( m_SayText, SayText );

int CHudSayText :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( SayText );

	InitHUDData();

	m_HUD_saytext =			gEngfuncs.pfnRegisterVariable( "hud_saytext", "1", 0 );
	m_HUD_saytext_sound =	gEngfuncs.pfnRegisterVariable( "hud_saytext_sound", "0", FCVAR_ARCHIVE);
	m_HUD_saytext_time =	gEngfuncs.pfnRegisterVariable( "hud_saytext_time", "5", 0 );


	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission

	return 1;
}


void CHudSayText :: InitHUDData( void )
{
	memset( g_szLineBuffer, 0, sizeof g_szLineBuffer );
	memset( g_pflNameColors, 0, sizeof g_pflNameColors );
	memset( g_iNameLengths, 0, sizeof g_iNameLengths );
}

int CHudSayText :: VidInit( void )
{
	return 1;
}


int ScrollTextUp( void )
{
	g_szLineBuffer[MAX_LINES][0] = 0;
	memmove( g_szLineBuffer[0], g_szLineBuffer[1], sizeof(g_szLineBuffer) - sizeof(g_szLineBuffer[0]) ); // overwrite the first line
	memmove( &g_pflNameColors[0], &g_pflNameColors[1], sizeof(g_pflNameColors) - sizeof(g_pflNameColors[0]) );
	memmove( &g_iNameLengths[0], &g_iNameLengths[1], sizeof(g_iNameLengths) - sizeof(g_iNameLengths[0]) );
	g_szLineBuffer[MAX_LINES-1][0] = 0;

	if ( g_szLineBuffer[0][0] == ' ' ) // also scroll up following lines
	{
		g_szLineBuffer[0][0] = 2;
		return 1 + ScrollTextUp();
	}

	return 1;
}

int CHudSayText::Draw(float flTime)
{
	int y = Y_START;

	if ((gViewPort && gViewPort->AllowedToPrintText() == FALSE) || !m_HUD_saytext->value)
		return 1;

	// make sure the scrolltime is within reasonable bounds,  to guard against the clock being reset
	flScrollTime = min(flScrollTime, flTime + m_HUD_saytext_time->value);

	if (flScrollTime <= flTime)
	{
		if (*g_szLineBuffer[0])
		{
			flScrollTime = flTime + m_HUD_saytext_time->value;
			// push the console up
			ScrollTextUp();
		}
		else
		{ // buffer is empty,  just disable drawing of this section
			m_iFlags &= ~HUD_ACTIVE;
		}
	}

	for (int i = 0; i < MAX_LINES; i++)
	{
		gEngfuncs.pfnDrawSetTextColor(1.0f, 160.0f / 255.0f, 0.0f);
		if (*g_szLineBuffer[i])
		{
			if (*g_szLineBuffer[i] == 2 && g_pflNameColors[i])
			{
				// це повідомлення гравця з ніком
				char buf[MAX_PLAYER_NAME_LENGTH + 32];

				// копіюємо частину з ніком
				strncpy(buf, g_szLineBuffer[i], min(g_iNameLengths[i], MAX_PLAYER_NAME_LENGTH + 31));
				buf[min(g_iNameLengths[i], MAX_PLAYER_NAME_LENGTH + 31)] = 0;

				// малюємо нік в кольорі гравця використовуючи спеціальну функцію
				int x = gHUD.DrawConsoleStringWithColorTags(
					LINE_START,
					y,
					buf + 1, // пропускаємо контрольний символ
					true,
					g_pflNameColors[i][0],
					g_pflNameColors[i][1],
					g_pflNameColors[i][2]
				);

				// малюємо решту тексту після ніка
				char* remaining_text = g_szLineBuffer[i] + g_iNameLengths[i];

				if (remaining_text && *remaining_text)
				{
					if (color_tags::contains_color_tags(remaining_text))
					{
						// є кольорові теги - використовуємо спеціальну функцію
						gHUD.DrawConsoleStringWithColorTags(x, y, remaining_text);
					}
					else
					{
						// немає кольорових тегів - малюємо оранжевим
						gEngfuncs.pfnDrawSetTextColor(1.0f, 160.0f / 255.0f, 0.0f);
						DrawConsoleString(x, y, remaining_text);
					}
				}
			}
			else
			{
				// звичайне повідомлення без ніка
				if (color_tags::contains_color_tags(g_szLineBuffer[i]))
				{
					gHUD.DrawConsoleStringWithColorTags(LINE_START, y, g_szLineBuffer[i], i);
				}
				else
				{
					// встановлюємо оранжевий колір для звичайного тексту
					gEngfuncs.pfnDrawSetTextColor(1.0f, 160.0f / 255.0f, 0.0f);
					DrawConsoleString(LINE_START, y, g_szLineBuffer[i]);
				}
			}
		}

		y += line_height;
	}

	return 1;
}

int CHudSayText :: MsgFunc_SayText( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int client_index = READ_BYTE();		// the client who spoke the message
	SayTextPrint( READ_STRING(), iSize - 1,  client_index );
	
	return 1;
}

void CHudSayText :: SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex )
{
	ConsolePrint( pszBuf );

	int i;
	// find an empty string slot
	for ( i = 0; i < MAX_LINES; i++ )
	{
		if ( ! *g_szLineBuffer[i] )
			break;
	}
	if ( i == MAX_LINES )
	{
		// force scroll buffer up
		ScrollTextUp();
		i = MAX_LINES - 1;
	}

	g_iNameLengths[i] = 0;
	g_pflNameColors[i] = NULL;

	// if it's a say message, search for the players name in the string
	if ( *pszBuf == 2 && clientIndex > 0 )
	{
		gEngfuncs.pfnGetPlayerInfo( clientIndex, &g_PlayerInfoList[clientIndex] );
		const char *pName = g_PlayerInfoList[clientIndex].name;

		if ( pName )
		{
			const char *nameInString = strstr( pszBuf, pName );

			if ( nameInString )
			{
				g_iNameLengths[i] = strlen( pName ) + (nameInString - pszBuf);
				g_pflNameColors[i] = GetClientColor( clientIndex );
			}
		}
	}

	strncpy( g_szLineBuffer[i], pszBuf, max(iBufSize , MAX_CHARS_PER_LINE) );

	// make sure the text fits in one line
	EnsureTextFitsInOneLineAndWrapIfHaveTo( i );

	// Set scroll time
	if ( i == 0 )
	{
		flScrollTime = gHUD.m_flTime + m_HUD_saytext_time->value;
	}

	m_iFlags |= HUD_ACTIVE;

	if (m_HUD_saytext_sound->value > 0.0f)
		PlaySound( "misc/talk.wav", m_HUD_saytext_sound->value);

	Y_START = ScreenHeight - 60 - ( line_height * (MAX_LINES+2) );
}

void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
    int line_width = 0;

    // Для точного вимірювання тексту з кольоровими тегами, спочатку видаляємо теги
    char clean_text[MAX_CHARS_PER_LINE];
    color_tags::strip_color_tags(clean_text, g_szLineBuffer[line], sizeof(clean_text));

    GetConsoleStringSize(clean_text, &line_width, &line_height);

    if ((line_width + LINE_START) > MAX_LINE_WIDTH)
    { // string is too long to fit on line
        int length = LINE_START;
        int tmp_len = 0;
        char* last_break = NULL;

        for (char* x = g_szLineBuffer[line]; *x != 0; x++)
        {
            // Пропускаємо кольорові теги при підрахунку довжини
            if (x[0] == '^' && x[1] >= '0' && x[1] <= '9')
            {
                // Перевіряємо що це не частина імені гравця
                if (g_szLineBuffer[line][0] != 2 || g_szLineBuffer[line] + g_iNameLengths[line] <= x)
                {
                    x++; // пропускаємо символ кольору
                    if (*x != 0) x++; // пропускаємо цифру
                    if (*x == 0) break;
                    continue;
                }
            }

            // Обробка старих кольорових тегів
            if (x[0] == '/' && x[1] == '(')
            {
                x += 2;
                while (*x != 0 && *x != ')')
                    x++;
                if (*x != 0) x++;
                if (*x == 0) break;
            }

            char buf[2];
            buf[1] = 0;

            if (*x == ' ' && x != g_szLineBuffer[line])
                last_break = x;

            buf[0] = *x;
            GetConsoleStringSize(buf, &tmp_len, &line_height);
            length += tmp_len;

            if (length > MAX_LINE_WIDTH)
            {
                if (!last_break)
                    last_break = x - 1;

                x = last_break;

                // find an empty string slot
                int j;
                do
                {
                    for (j = 0; j < MAX_LINES; j++)
                    {
                        if (!*g_szLineBuffer[j])
                            break;
                    }
                    if (j == MAX_LINES)
                    {
                        int linesmoved = ScrollTextUp();
                        line -= linesmoved;
                        last_break = last_break - (sizeof(g_szLineBuffer[0]) * linesmoved);
                    }
                } while (j == MAX_LINES);

                // copy remaining string into next buffer
                if ((char)*last_break == (char)' ')
                {
                    int linelen = strlen(g_szLineBuffer[j]);
                    int remaininglen = strlen(last_break);

                    if ((linelen + remaininglen) < MAX_CHARS_PER_LINE)
                        strcat(g_szLineBuffer[j], last_break);
                }
                else
                {
                    if ((strlen(g_szLineBuffer[j]) + strlen(last_break) + 1) < MAX_CHARS_PER_LINE)
                    {
                        strcat(g_szLineBuffer[j], " ");
                        strcat(g_szLineBuffer[j], last_break);
                    }
                }

                *last_break = 0; // cut off the last string

                EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
                break;
            }
        }
    }
}