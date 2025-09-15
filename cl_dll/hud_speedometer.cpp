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
	hud_speedometer_height = CVAR_CREATE("hud_speedometer_height", "0", FCVAR_ARCHIVE);

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
	if (hud_speedometer_below_cross->value != 0.0f)
	{
		y = ScreenHeight / 2 + gHUD.m_iFontHeight / 2;

	}
	else if (hud_speedometer_height->value > 0.0f)
	{
		y = hud_speedometer_height->value;
	}
	else
	{
		cvar_t* hud_jumpspeed = gEngfuncs.pfnGetCvarPointer("hud_jumpspeed");
		cvar_t* hud_jumpspeed_below_cross = gEngfuncs.pfnGetCvarPointer("hud_jumpspeed_below_cross");
		cvar_t* hud_jumpspeed_height = gEngfuncs.pfnGetCvarPointer("hud_jumpspeed_height");
		if (hud_jumpspeed_below_cross->value != 0.0f || hud_jumpspeed_height->value != 0.0f || hud_jumpspeed->value == 0.0f)
		{
			y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		}
		else if (hud_jumpspeed_below_cross->value == 0.0f || hud_jumpspeed_height->value == 0.0f)
		{
			y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2 - gHUD.m_iFontHeight;
		}
	}

	gHUD.DrawHudNumberCentered(x, y, speed, r, g, b);

	return 0;
}

void CHudSpeedometer::UpdateSpeed(const float velocity[2])
{
	speed = std::round(std::hypot(velocity[0], velocity[1]));
}