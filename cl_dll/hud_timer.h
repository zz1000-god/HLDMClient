#pragma once
#include "net.h" // Äîäàéòå öåé ðÿäîê äëÿ NetSocket

class CHudTimer : public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw(float time);

	int MsgFunc_Timer(const char* name, int size, void* buf);
	void Think();
	// void SyncTimerLocal(float fTime); // Ìîæíà çàëèøèòè, ÿêùî º îêðåìà ðåàë³çàö³ÿ, àáî ³íòåãðóâàòè â SyncTimer

private:
	void SyncTimer(float fTime);
	void DoResync();
	void SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency); // Îãîëîøåííÿ íîâî¿ ôóíêö³¿

	// Message data
	int seconds_total;
	int seconds_passed;
	float draw_until;


	// Sync data
	float m_flEndTime;
	float m_flEffectiveTime; // Âèêîðèñòîâóºòüñÿ, ÿêùî m_flSynced == false
	float m_flNextSyncTime;
	bool m_flSynced;
	bool m_bDelayTimeleftReading;

	// CVars
	cvar_t* hud_timer;
	cvar_t* hud_timer_height;
	cvar_t* m_pCvarHudTimerSync;
	cvar_t* m_pCvarMpTimelimit;
	cvar_t* m_pCvarMpTimeleft;

	// Íîâ³ çì³íí³ äëÿ A2S_RULES ñèíõðîí³çàö³¿
	char m_szPacketBuffer[2048]; // Áóôåð äëÿ çáåðåæåííÿ â³äïîâ³ä³ (ìîæëèâî, ôðàãìåíòîâàíî¿)
	int m_iReceivedSize;         // Çàãàëüíèé ðîçì³ð îòðèìàíî¿ â³äïîâ³ä³
	int m_iResponceID;           // ID ïîòî÷íî¿ â³äïîâ³ä³ (äëÿ ôðàãìåíòîâàíèõ ïàêåò³â)
	int m_iReceivedPackets;      // Á³òîâà ìàñêà îòðèìàíèõ ôðàãìåíòîâàíèõ ïàêåò³â
	int m_iReceivedPacketsCount; // Ê³ëüê³ñòü îòðèìàíèõ ôðàãìåíòîâàíèõ ïàêåò³â
};
