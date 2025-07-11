#pragma once
#include "net.h" // ������� ��� ����� ��� NetSocket

class CHudTimer : public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw(float time);

	int MsgFunc_Timer(const char* name, int size, void* buf);
	void Think();
	// void SyncTimerLocal(float fTime); // ����� ��������, ���� � ������ ���������, ��� ����������� � SyncTimer

private:
	void SyncTimer(float fTime);
	void DoResync();
	void SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency); // ���������� ���� �������

	// Message data
	int seconds_total;
	int seconds_passed;
	float draw_until;


	// Sync data
	float m_flEndTime;
	float m_flEffectiveTime; // ���������������, ���� m_flSynced == false
	float m_flNextSyncTime;
	bool m_flSynced;
	bool m_bDelayTimeleftReading;

	// CVars
	cvar_t* hud_timer;
	cvar_t* m_pCvarHudTimerSync;
	cvar_t* m_pCvarMpTimelimit;
	cvar_t* m_pCvarMpTimeleft;

	// ��� ���� ��� A2S_RULES ������������
	char m_szPacketBuffer[2048]; // ����� ��� ���������� ������ (�������, �������������)
	int m_iReceivedSize;         // ��������� ����� �������� ������
	int m_iResponceID;           // ID ������� ������ (��� �������������� ������)
	int m_iReceivedPackets;      // ������ ����� ��������� �������������� ������
	int m_iReceivedPacketsCount; // ʳ������ ��������� �������������� ������
};