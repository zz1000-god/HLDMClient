#pragma once
#include "net.h"

class CHudTimer : public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw(float time);

	int MsgFunc_Timer(const char* name, int size, void* buf);
	void Think();
	
private:
	void SyncTimer(float fTime);
	void DoResync();
	void SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency);
	void DrawNextMap(float time);

	// Message data
	int seconds_total;
	int seconds_passed;
	float draw_until;


	// Sync data
	float m_flEndTime;
	float m_flEffectiveTime;
	float m_flNextSyncTime;
	bool m_flSynced;
	bool m_bDelayTimeleftReading;

	// CVars
	cvar_t* hud_timer;
	cvar_t* hud_timer_height;
	cvar_t* hud_timer_24f;
	cvar_t* hud_timer_show_seconds;
	cvar_t* hud_nextmap;
	cvar_t* m_pCvarHudTimerSync;
	cvar_t* m_pCvarMpTimelimit;
	cvar_t* m_pCvarMpTimeleft;


	char m_szPacketBuffer[2048]; 
	int m_iReceivedSize;         
	int m_iResponceID;         
	int m_iReceivedPackets;    
	int m_iReceivedPacketsCount;

	char m_szNextMap[64];
};