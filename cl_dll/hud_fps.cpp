#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string>

int CHudFPS::Init()
{
    m_flLastTime = gEngfuncs.GetClientTime();
    m_iFrameCount = 0;
    m_flFPS = 0.0f;

    m_iFlags = HUD_ACTIVE;

    hud_fps = CVAR_CREATE("hud_fps", "1", FCVAR_ARCHIVE);
    hud_fps_pos = CVAR_CREATE("hud_fps_pos", "1400 0", FCVAR_ARCHIVE);
    hud_fps_updaterate = CVAR_CREATE("hud_fps_updaterate", "1.0", FCVAR_ARCHIVE);
    hud_fps_precision = CVAR_CREATE("hud_fps_precision", "0", FCVAR_ARCHIVE);
    hud_fps_yellowflor = CVAR_CREATE("hud_fps_yellowflor", "21", FCVAR_ARCHIVE);
    hud_fps_greenflor = CVAR_CREATE("hud_fps_greenflor", "60", FCVAR_ARCHIVE);

    gHUD.AddHudElem(this);
    return 0;
}

int CHudFPS::VidInit()
{
    m_flLastTime = gEngfuncs.GetClientTime();
    m_iFrameCount = 0;
    m_flFPS = 0.0f;
    return 1;
}

int CHudFPS::Draw(float flTime)
{
    if (hud_fps->value == 0)
        return 0;

    float flCurrentTime = gEngfuncs.GetClientTime();
    m_iFrameCount++;

    float updateInterval = hud_fps_updaterate ? hud_fps_updaterate->value : 1.0f;
    if (updateInterval < 0.01f) updateInterval = 0.01f;

    if (flCurrentTime - m_flLastTime >= updateInterval)
    {
        m_flFPS = m_iFrameCount / (flCurrentTime - m_flLastTime);

        cvar_t* fpsMax = gEngfuncs.pfnGetCvarPointer("fps_max");
        if (fpsMax && fpsMax->value > 0.0f)
        {
            if (m_flFPS > fpsMax->value)
                m_flFPS = fpsMax->value;
        }
        m_iFrameCount = 0;
        m_flLastTime = flCurrentTime;
    }

    int digits = hud_fps_precision ? (int)hud_fps_precision->value : 0;
    if (digits < 0) digits = 0;
    if (digits > 4) digits = 4;

    char sz[64];
    if (digits == 0)
    {
        sprintf(sz, "FPS: %d", (int)(m_flFPS + 0.5f));
    }
    else
    {
        char format[16];
        sprintf(format, "FPS: %%0.%df", digits);
        sprintf(sz, format, m_flFPS);
    }

    int x, y;
    if (sscanf(hud_fps_pos->string, "%d %d", &x, &y) != 2) {
        x = 0;
        y = 0;
    }

    if(hud_fps->value == 1){
        int r, g, b;

        if (m_flFPS < hud_fps_yellowflor->value) {
            r = 255;
            g = 0;
            b = 0;
        }
        else if (m_flFPS < hud_fps_greenflor->value) {
            r = 255;
            g = 255;
            b = 0;
        }
        else {
            r = 0;
            g = 255;
            b = 0;
        }
        gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
        DrawConsoleString(x, y, sz);
    }
    else if (hud_fps->value == 2){
        int r, g, b;
        UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);

        if (m_flFPS < hud_fps_yellowflor->value) {
            r = 255;
            g = 0;
            b = 0;
        }
        gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
        DrawConsoleString(x, y, sz);
    }
    else if (hud_fps->value == 3) {
        int r, g, b;
        UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
        gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
        DrawConsoleString(x, y, sz);
    }
    return 0;
}
