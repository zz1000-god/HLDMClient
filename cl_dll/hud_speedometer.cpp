#include <cmath>
#include <cstring>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "hudgl.h"

int CHudSpeedometer::Init()
{
	m_iFlags = HUD_ACTIVE;

	hud_speedometer = CVAR_CREATE("hud_speedometer", "0", FCVAR_ARCHIVE);
	hud_speedometer_below_cross = CVAR_CREATE("hud_speedometer_below_cross", "0", FCVAR_ARCHIVE);
	hud_speedometer_pos = CVAR_CREATE("hud_speedometer_pos", "0 0", FCVAR_ARCHIVE);

	gHUD.AddHudElem(this);
	return 0;
}

int CHudSpeedometer::VidInit()
{
	return 1;
}

int CHudSpeedometer::Draw(float time)
{
	if (hud_speedometer->value == 0.0f)
		return 0;

	int r, g, b;
	UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);

	int y;
	int x = ScreenWidth / 2;
	int customX = 0, customY = 0;
	int useCustomPos = false;
	if (hud_speedometer_pos && strcmp(hud_speedometer_pos->string, "0 0") != 0)
	{
		if (sscanf(hud_speedometer_pos->string, "%d %d", &customX, &customY) == 2)
			useCustomPos = true;
	}

	if (useCustomPos) // default position
	{
		x = customX;
		y = customY;
	}
	else
	{
		if (hud_speedometer_below_cross->value != 0.0f)
			y = ScreenHeight / 2 + gHUD.m_iFontHeight / 2;
		else
		{
			cvar_t* hud_jumpspeed = gEngfuncs.pfnGetCvarPointer("hud_jumpspeed");
			if (hud_jumpspeed && hud_jumpspeed->value == 0.0f)
			{
				y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
			}
			else
			{
				y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2 - gHUD.m_iFontHeight;
			}
		}
	}

	gHUD.DrawHudNumberCentered(x, y, speed, r, g, b);

	return 0;
}

void CHudSpeedometer::UpdateSpeed(const float velocity[2])
{
	speed = std::round(std::hypot(velocity[0], velocity[1]));
}
