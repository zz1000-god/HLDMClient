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
// Message.cpp
//
// implementation of CHudMessage class
//

#include "hud.h"
#include "cl_util.h"
#include "commonmacros.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

DECLARE_MESSAGE( m_Message, HudText )
DECLARE_MESSAGE( m_Message, GameTitle )

// 1 Global client_textmessage_t for custom messages that aren't in the titles.txt
client_textmessage_t	g_pCustomMessage;
char *g_pCustomName = "Custom";
char g_pCustomText[1024];

int CHudMessage::Init(void)
{
	HOOK_MESSAGE( HudText );
	HOOK_MESSAGE( GameTitle );

	gHUD.AddHudElem(this);

	Reset();

	return 1;
};

int CHudMessage::VidInit( void )
{
	m_HUD_title_half = gHUD.GetSpriteIndex( "title_half" );
	m_HUD_title_life = gHUD.GetSpriteIndex( "title_life" );

	return 1;
};


void CHudMessage::Reset( void )
{
 	memset( m_pMessages, 0, sizeof( m_pMessages[0] ) * maxHUDMessages );
	memset( m_startTime, 0, sizeof( m_startTime[0] ) * maxHUDMessages );
	
	m_bEndAfterMessage = false;
	m_gameTitleTime = 0;
	m_pGameTitle = NULL;
}


float CHudMessage::FadeBlend( float fadein, float fadeout, float hold, float localTime )
{
	float fadeTime = fadein + hold;
	float fadeBlend;

	if ( localTime < 0 )
		return 0;

	if ( localTime < fadein )
	{
		fadeBlend = 1 - ((fadein - localTime) / fadein);
	}
	else if ( localTime > fadeTime )
	{
		if ( fadeout > 0 )
			fadeBlend = 1 - ((localTime - fadeTime) / fadeout);
		else
			fadeBlend = 0;
	}
	else
		fadeBlend = 1;

	return fadeBlend;
}


int	CHudMessage::XPosition( float x, int width, int totalWidth )
{
	int xPos;

	if ( x == -1 )
	{
		xPos = (ScreenWidth - width) / 2;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0 + x) * ScreenWidth - totalWidth;	// Alight right
		else
			xPos = x * ScreenWidth;
	}

	if ( xPos + width > ScreenWidth )
		xPos = ScreenWidth - width;
	else if ( xPos < 0 )
		xPos = 0;

	return xPos;
}


int CHudMessage::YPosition( float y, int height )
{
	int yPos;

	if ( y == -1 )	// Centered?
		yPos = (ScreenHeight - height) * 0.5;
	else
	{
		// Alight bottom?
		if ( y < 0 )
			yPos = (1.0 + y) * ScreenHeight - height;	// Alight bottom
		else // align top
			yPos = y * ScreenHeight;
	}

	if ( yPos + height > ScreenHeight )
		yPos = ScreenHeight - height;
	else if ( yPos < 0 )
		yPos = 0;

	return yPos;
}


void CHudMessage::MessageScanNextChar( void )
{
	int srcRed, srcGreen, srcBlue, destRed, destGreen, destBlue;
	int blend;

	srcRed = m_parms.pMessage->r1;
	srcGreen = m_parms.pMessage->g1;
	srcBlue = m_parms.pMessage->b1;
	destRed = 0;
	destGreen = 0;
	destBlue = 0;
	blend = 0;	// Pure source

	switch( m_parms.pMessage->effect )
	{
	// Fade-in / Fade-out
	case 0:
	case 1:
		destRed = destGreen = destBlue = 0;
		blend = m_parms.fadeBlend;
		break;

	case 2:
		m_parms.charTime += m_parms.pMessage->fadein;
		if ( m_parms.charTime > m_parms.time )
		{
			srcRed = srcGreen = srcBlue = 0;
			blend = 0;	// pure source
		}
		else
		{
			float deltaTime = m_parms.time - m_parms.charTime;

			destRed = destGreen = destBlue = 0;
			if ( m_parms.time > m_parms.fadeTime )
			{
				blend = m_parms.fadeBlend;
			}
			else if ( deltaTime > m_parms.pMessage->fxtime )
				blend = 0;	// pure dest
			else
			{
				destRed = m_parms.pMessage->r2;
				destGreen = m_parms.pMessage->g2;
				destBlue = m_parms.pMessage->b2;
				blend = 255 - (deltaTime * (1.0/m_parms.pMessage->fxtime) * 255.0 + 0.5);
			}
		}
		break;
	}
	if ( blend > 255 )
		blend = 255;
	else if ( blend < 0 )
		blend = 0;

	m_parms.r = ((srcRed * (255-blend)) + (destRed * blend)) >> 8;
	m_parms.g = ((srcGreen * (255-blend)) + (destGreen * blend)) >> 8;
	m_parms.b = ((srcBlue * (255-blend)) + (destBlue * blend)) >> 8;

	if ( m_parms.pMessage->effect == 1 && m_parms.charTime != 0 )
	{
		if ( m_parms.x >= 0 && m_parms.y >= 0 && (m_parms.x + gHUD.m_scrinfo.charWidths[ m_parms.text ]) <= ScreenWidth )
			TextMessageDrawChar( m_parms.x, m_parms.y, m_parms.text, m_parms.pMessage->r2, m_parms.pMessage->g2, m_parms.pMessage->b2 );
	}
}


void CHudMessage::MessageScanStart( void )
{
	switch( m_parms.pMessage->effect )
	{
	// Fade-in / out with flicker
	case 1:
	case 0:
		m_parms.fadeTime = m_parms.pMessage->fadein + m_parms.pMessage->holdtime;
		

		if ( m_parms.time < m_parms.pMessage->fadein )
		{
			m_parms.fadeBlend = ((m_parms.pMessage->fadein - m_parms.time) * (1.0/m_parms.pMessage->fadein) * 255);
		}
		else if ( m_parms.time > m_parms.fadeTime )
		{
			if ( m_parms.pMessage->fadeout > 0 )
				m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
			else
				m_parms.fadeBlend = 255; // Pure dest (off)
		}
		else
			m_parms.fadeBlend = 0;	// Pure source (on)
		m_parms.charTime = 0;

		if ( m_parms.pMessage->effect == 1 && (rand()%100) < 10 )
			m_parms.charTime = 1;
		break;

	case 2:
		m_parms.fadeTime = (m_parms.pMessage->fadein * m_parms.length) + m_parms.pMessage->holdtime;
		
		if ( m_parms.time > m_parms.fadeTime && m_parms.pMessage->fadeout > 0 )
			m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
		else
			m_parms.fadeBlend = 0;
		break;
	}
}


void CHudMessage::MessageDrawScan(client_textmessage_t* pMessage, float time)
{
	int i, j, length, width;
	const char* pText;
	unsigned char line[80];

	pText = pMessage->pMessage;

	// Count lines and calculate total width without color tags
	m_parms.lines = 1;
	m_parms.time = time;
	m_parms.pMessage = pMessage;
	length = 0;
	width = 0;
	m_parms.totalWidth = 0;

	// First pass: strip color tags and count actual display width
	char tempMessage[512];
	color_tags::strip_color_tags(tempMessage, pMessage->pMessage, sizeof(tempMessage));

	pText = tempMessage;
	while (*pText)
	{
		if (*pText == '\n')
		{
			m_parms.lines++;
			if (width > m_parms.totalWidth)
				m_parms.totalWidth = width;
			width = 0;
		}
		else
			width += gHUD.m_scrinfo.charWidths[*pText];
		pText++;
		length++;
	}

	// Final width check
	if (width > m_parms.totalWidth)
		m_parms.totalWidth = width;

	m_parms.length = length;
	m_parms.totalHeight = (m_parms.lines * gHUD.m_scrinfo.iCharHeight);
	m_parms.y = YPosition(pMessage->y, m_parms.totalHeight);

	pText = pMessage->pMessage;
	m_parms.charTime = 0;

	MessageScanStart();

	// Track global character index for animations
	int globalCharIndex = 0;

	for (i = 0; i < m_parms.lines; i++)
	{
		m_parms.lineLength = 0;
		m_parms.width = 0;

		char rawLine[80];
		int rawLen = 0;

		// Extract raw line with color tags
		while (*pText && *pText != '\n' && rawLen < ARRAYSIZE(rawLine) - 1)
		{
			rawLine[rawLen++] = *pText++;
		}
		rawLine[rawLen] = '\0';

		// Skip newline if present
		if (*pText == '\n')
			pText++;

		// Calculate width without color tags for positioning
		char cleanLine[80];
		color_tags::strip_color_tags(cleanLine, rawLine, sizeof(cleanLine));

		m_parms.width = 0;
		for (int ch = 0; cleanLine[ch] != '\0'; ++ch)
			m_parms.width += gHUD.m_scrinfo.charWidths[static_cast<unsigned char>(cleanLine[ch])];

		m_parms.x = XPosition(pMessage->x, m_parms.width, m_parms.totalWidth);

		// Now render with color tags and animations
		char lineCopy[80];
		strncpy(lineCopy, rawLine, sizeof(lineCopy));
		lineCopy[sizeof(lineCopy) - 1] = '\0';

		int xPos = m_parms.x;

		color_tags::for_each_colored_substr(lineCopy,
			[&](const char* substr, bool customColor, int r, int g, int b) {
				// If no custom color, use original message colors
				if (!customColor) {
					r = m_parms.pMessage->r1;
					g = m_parms.pMessage->g1;
					b = m_parms.pMessage->b1;
				}

				for (int k = 0; substr[k] != '\0'; ++k) {
					unsigned char ch = substr[k];
					int charWidth = gHUD.m_scrinfo.charWidths[ch];

					// Set up character parameters for animation
					m_parms.text = ch;
					m_parms.x = xPos;

					// For effect 2 (typewriter), calculate per-character timing
					if (m_parms.pMessage->effect == 2) {
						m_parms.charTime = globalCharIndex * m_parms.pMessage->fadein;
					}

					// Calculate animated colors
					MessageScanNextChar();

					int finalR, finalG, finalB;

					// Apply animation effects based on message effect type
					switch (m_parms.pMessage->effect) {
						case 0: // Fade in/out
						case 1: // Flicker
							// Blend animation with current color
							{
								int blend = m_parms.fadeBlend;
								if (blend > 255) blend = 255;
								else if (blend < 0) blend = 0;

								finalR = ((r * (255 - blend)) + (0 * blend)) >> 8;
								finalG = ((g * (255 - blend)) + (0 * blend)) >> 8;
								finalB = ((b * (255 - blend)) + (0 * blend)) >> 8;
							}
							break;

						case 2: // Typewriter
							// For typewriter effect, check if character should be visible
							if (m_parms.charTime > m_parms.time) {
								// Character not yet visible
								finalR = finalG = finalB = 0;
							} else {
								// Character is visible, apply fade and color tags
								int blend = m_parms.fadeBlend;
								if (blend > 255) blend = 255;
								else if (blend < 0) blend = 0;

								// Blend color tag colors with fade
								finalR = ((r * (255 - blend)) + (0 * blend)) >> 8;
								finalG = ((g * (255 - blend)) + (0 * blend)) >> 8;
								finalB = ((b * (255 - blend)) + (0 * blend)) >> 8;
							}
							break;

						default:
							// No animation, use color tag colors directly
							finalR = r;
							finalG = g;
							finalB = b;
							break;
					}

					// Draw character if within screen bounds and visible
					if (xPos >= 0 && m_parms.y >= 0 && xPos < ScreenWidth && m_parms.y < ScreenHeight) {
						// For effect 2, only draw if character time has passed
						if (m_parms.pMessage->effect != 2 || m_parms.charTime <= m_parms.time) {
							TextMessageDrawChar(xPos, m_parms.y, ch, finalR, finalG, finalB);
						}
					}

					xPos += charWidth;
					globalCharIndex++;
				}
			});

		m_parms.y += gHUD.m_scrinfo.iCharHeight;
	}
}

int CHudMessage::Draw( float fTime )
{
	int i, drawn;
	client_textmessage_t *pMessage;
	float endTime;

	drawn = 0;

	if ( m_gameTitleTime > 0 )
	{
		float localTime = gHUD.m_flTime - m_gameTitleTime;
		float brightness;

		// Maybe timer isn't set yet
		if ( m_gameTitleTime > gHUD.m_flTime )
			m_gameTitleTime = gHUD.m_flTime;

		if ( localTime > (m_pGameTitle->fadein + m_pGameTitle->holdtime + m_pGameTitle->fadeout) )
			m_gameTitleTime = 0;
		else
		{
			brightness = FadeBlend( m_pGameTitle->fadein, m_pGameTitle->fadeout, m_pGameTitle->holdtime, localTime );

			int halfWidth = gHUD.GetSpriteRect(m_HUD_title_half).right - gHUD.GetSpriteRect(m_HUD_title_half).left;
			int fullWidth = halfWidth + gHUD.GetSpriteRect(m_HUD_title_life).right - gHUD.GetSpriteRect(m_HUD_title_life).left;
			int fullHeight = gHUD.GetSpriteRect(m_HUD_title_half).bottom - gHUD.GetSpriteRect(m_HUD_title_half).top;

			int x = XPosition( m_pGameTitle->x, fullWidth, fullWidth );
			int y = YPosition( m_pGameTitle->y, fullHeight );


			SPR_Set( gHUD.GetSprite(m_HUD_title_half), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(m_HUD_title_half) );

			SPR_Set( gHUD.GetSprite(m_HUD_title_life), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x + halfWidth, y, &gHUD.GetSpriteRect(m_HUD_title_life) );

			drawn = 1;
		}
	}
	// Fixup level transitions
	for ( i = 0; i < maxHUDMessages; i++ )
	{
		// Assume m_parms.time contains last time
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];
			if ( m_startTime[i] > gHUD.m_flTime )
				m_startTime[i] = gHUD.m_flTime + m_parms.time - m_startTime[i] + 0.2;	// Server takes 0.2 seconds to spawn, adjust for this
		}
	}

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];

			// This is when the message is over
			switch( pMessage->effect )
			{
			case 0:
			case 1:
				endTime = m_startTime[i] + pMessage->fadein + pMessage->fadeout + pMessage->holdtime;
				break;
			
			// Fade in is per character in scanning messages
			case 2:
				endTime = m_startTime[i] + (pMessage->fadein * strlen( pMessage->pMessage )) + pMessage->fadeout + pMessage->holdtime;
				break;
			}

			if ( fTime <= endTime )
			{
				float messageTime = fTime - m_startTime[i];

				// Draw the message
				// effect 0 is fade in/fade out
				// effect 1 is flickery credits
				// effect 2 is write out (training room)
				MessageDrawScan( pMessage, messageTime );

				drawn++;
			}
			else
			{
				// The message is over
				m_pMessages[i] = NULL;

				if (m_bEndAfterMessage)
				{
					// leave game
					gEngfuncs.pfnClientCmd("wait\nwait\nwait\nwait\nwait\nwait\nwait\ndisconnect\n");
				}
			}
		}
	}

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;
	// Don't call until we get another message
	if ( !drawn )
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}


void CHudMessage::MessageAdd( const char *pName, float time )
{
	int i,j;
	client_textmessage_t *tempMessage;

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			// Trim off a leading # if it's there
			if ( pName[0] == '#' ) 
				tempMessage = TextMessageGet( pName+1 );
			else
				tempMessage = TextMessageGet( pName );
			// If we couldnt find it in the titles.txt, just create it
			if ( !tempMessage )
			{
				g_pCustomMessage.effect = 2;
				g_pCustomMessage.r1 = g_pCustomMessage.g1 = g_pCustomMessage.b1 = g_pCustomMessage.a1 = 100;
				g_pCustomMessage.r2 = 240;
				g_pCustomMessage.g2 = 110;
				g_pCustomMessage.b2 = 0;
				g_pCustomMessage.a2 = 0;
				g_pCustomMessage.x = -1;		// Centered
				g_pCustomMessage.y = 0.7;
				g_pCustomMessage.fadein = 0.01;
				g_pCustomMessage.fadeout = 1.5;
				g_pCustomMessage.fxtime = 0.25;
				g_pCustomMessage.holdtime = 5;
				g_pCustomMessage.pName = g_pCustomName;
				strcpy( g_pCustomText, pName );
				g_pCustomMessage.pMessage = g_pCustomText;

				tempMessage = &g_pCustomMessage;
			}

			for ( j = 0; j < maxHUDMessages; j++ )
			{
				if ( m_pMessages[j] )
				{
					// is this message already in the list
					if ( !strcmp( tempMessage->pMessage, m_pMessages[j]->pMessage ) )
					{
						return;
					}

					// get rid of any other messages in same location (only one displays at a time)
					if ( fabs( tempMessage->y - m_pMessages[j]->y ) < 0.0001 )
					{
						if ( fabs( tempMessage->x - m_pMessages[j]->x ) < 0.0001 )
						{
							m_pMessages[j] = NULL;
						}
					}
				}
			}

			m_pMessages[i] = tempMessage;
			m_startTime[i] = time;
			return;
		}
	}
}


int CHudMessage::MsgFunc_HudText( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	char *pString = READ_STRING();

	bool bIsEnding = false;
	const char *HL1_ENDING_STR = "END3";

	if (strlen(pString) == strlen(HL1_ENDING_STR) && strcmp(HL1_ENDING_STR, pString) == 0)
	{
		m_bEndAfterMessage = true;
	}

	MessageAdd( pString, gHUD.m_flTime );
	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	if ( !(m_iFlags & HUD_ACTIVE) )
		m_iFlags |= HUD_ACTIVE;

	return 1;
}


int CHudMessage::MsgFunc_GameTitle( const char *pszName,  int iSize, void *pbuf )
{
	m_pGameTitle = TextMessageGet( "GAMETITLE" );
	if ( m_pGameTitle != NULL )
	{
		m_gameTitleTime = gHUD.m_flTime;

		// Turn on drawing
		if ( !(m_iFlags & HUD_ACTIVE) )
			m_iFlags |= HUD_ACTIVE;
	}

	return 1;
}

void CHudMessage::MessageAdd(client_textmessage_t * newMessage )
{
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	if ( !(m_iFlags & HUD_ACTIVE) )
		m_iFlags |= HUD_ACTIVE;
	
	for ( int i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			m_pMessages[i] = newMessage;
			m_startTime[i] = gHUD.m_flTime;
			return;
		}
	}

}
