#pragma once

class CHudFPS : public CHudBase
{
	cvar_t* hud_fps;
	cvar_t* hud_fps_pos;
	cvar_t* hud_fps_precision;
	cvar_t* hud_fps_yellowflor;
	cvar_t* hud_fps_greenflor;
	cvar_t* hud_fps_updaterate;
public: 
	int Init() override;
	int VidInit() override;
	int Draw(float time) override;

private:
	float m_flLastTime = 0.0f;
	int m_iFrameCount = 0;
	float m_flFPS = 0.0f;
};