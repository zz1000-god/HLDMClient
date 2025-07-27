#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "hud_timer.h"
#include <ctime>
#include "net_api.h" 
#include "net.h" 

#define NET_API gEngfuncs.pNetAPI // Макрос для доступу до NetAPI

enum RulesRequestStatus
{
    SOCKET_NONE = 0,
    SOCKET_IDLE = 1,
    SOCKET_AWAITING_CODE = 2,
    SOCKET_AWAITING_ANSWER = 3,
};
RulesRequestStatus g_eRulesRequestStatus = SOCKET_NONE;
NetSocket g_timerSocket = 0; 

DECLARE_MESSAGE(m_Timer, Timer);

static void unpack_seconds(int seconds_total, int& days, int& hours, int& minutes, int& seconds)
{
	constexpr int SECONDS_PER_MINUTE = 60;
	constexpr int SECONDS_PER_HOUR = SECONDS_PER_MINUTE * 60;
	constexpr int SECONDS_PER_DAY = SECONDS_PER_HOUR * 24;

	days = seconds_total / SECONDS_PER_DAY;
	seconds_total %= SECONDS_PER_DAY;

	hours = seconds_total / SECONDS_PER_HOUR;
	seconds_total %= SECONDS_PER_HOUR;

	minutes = seconds_total / SECONDS_PER_MINUTE;
	seconds_total %= SECONDS_PER_MINUTE;

	seconds = seconds_total;
}

int CHudTimer::Init()
{
	HOOK_MESSAGE(Timer);
	m_iFlags |= HUD_ACTIVE;
	
	hud_timer = CVAR_CREATE("hud_timer", "1", FCVAR_ARCHIVE);
	hud_timer_height = CVAR_CREATE("hud_timer_height", "0", FCVAR_ARCHIVE);
	m_pCvarHudTimerSync = CVAR_CREATE("hud_timer_sync", "1", FCVAR_ARCHIVE);
	gHUD.AddHudElem(this);
	
	// Initialize member variables
	seconds_total = 0;
	seconds_passed = 0;
	draw_until = 0.0f;
	
	// Initialize sync variables
	m_flEndTime = 0.0f;
	m_flEffectiveTime = 0.0f;
	m_flNextSyncTime = 0.0f;
	m_flSynced = false;
	m_bDelayTimeleftReading = true;
	
	return 1;
}

int CHudTimer::VidInit()
{
    m_iFlags |= HUD_ACTIVE;

    m_pCvarMpTimelimit = gEngfuncs.pfnGetCvarPointer("mp_timelimit");
    m_pCvarMpTimeleft = gEngfuncs.pfnGetCvarPointer("mp_timeleft");

    m_flEndTime = 0.0f;
    m_flEffectiveTime = 0.0f;
    m_flNextSyncTime = 0.0f;
    m_flSynced = false;
    m_bDelayTimeleftReading = true;

    m_iReceivedSize = 0;
    m_iResponceID = 0;
    m_iReceivedPackets = 0;
    m_iReceivedPacketsCount = 0;
    memset(m_szPacketBuffer, 0, sizeof(m_szPacketBuffer));

    if (g_timerSocket != 0)
    {
        NetCloseSocket(g_timerSocket);
        g_timerSocket = 0;
    }
    g_eRulesRequestStatus = SOCKET_NONE;

    return 1;
}

int CHudTimer::Draw(float time)
{
    int y = gHUD.m_scrinfo.iCharHeight;
    int x = ScreenWidth / 2;

	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return 0;

	// Handle local time display (hud_timer = 3)
	if (hud_timer->value == 3.0f) {
		time_t rawtime;
		struct tm* timeinfo;
		char str[64];
		
		::time(&rawtime);
		timeinfo = ::localtime(&rawtime);
		
		// Format: HH:MM:SS
		sprintf(str, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		
		int r, g, b;
		UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
        if (hud_timer_height->value > 0.0f) {
            y = hud_timer_height->value;
		}
		gHUD.DrawHudStringCentered(x, y, str, r, g, b);
		return 1;
	}

	// Handle message-based timer
	if (gHUD.m_flTime < draw_until) {
		if (hud_timer->value == 0.0f)
			return 0;

		char str[64];
		int seconds_to_draw = (hud_timer->value == 2.0f || seconds_total == 0)
			? seconds_passed
			: seconds_total - seconds_passed;

		int days, hours, minutes, seconds;
		unpack_seconds(seconds_to_draw, days, hours, minutes, seconds);

		if (days > 0)
			sprintf(str, "%d day%s %dh %dm %ds", days, (days > 1 ? "s" : ""), hours, minutes, seconds);
		else if (hours > 0)
			sprintf(str, "%dh %dm %ds", hours, minutes, seconds);
		else if (minutes > 0)
			sprintf(str, "%d:%02d", minutes, seconds);
		else if (seconds_to_draw >= 0)
			sprintf(str, "%d", seconds);
		else
			sprintf(str, "%d", seconds_to_draw); // overtime

		int r, g, b;
		UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
        if (hud_timer_height->value > 0.0f) {
            y = hud_timer_height->value;
		}
		gHUD.DrawHudStringCentered(x, y, str, r, g, b);
		return 1;
	}
	
	// Handle synced timer when no message timer is active
	if (hud_timer->value == 0.0f)
		return 0;
		
	// For synced timer (values 1 and 2), we need sync data
	if ((hud_timer->value == 1.0f || hud_timer->value == 2.0f) && !m_flSynced) {
		// If no sync data available, try to use basic time display
		char str[64];
		int seconds_to_draw = (int)time;
		
		if (hud_timer->value == 1.0f) {
			// Show remaining time based on mp_timelimit if available
			if (m_pCvarMpTimelimit && m_pCvarMpTimelimit->value > 0) {
				seconds_to_draw = (int)(m_pCvarMpTimelimit->value * 60 - time);
				if (seconds_to_draw < 0) seconds_to_draw = 0;
			} else {
				strcpy(str, " ");
				int r, g, b;
				UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
                if (hud_timer_height->value > 0.0f) {
                    y = hud_timer_height->value;
				}
				gHUD.DrawHudStringCentered(x, y, str, r, g, b);
				return 1;
			}

		
		} else {
			// hud_timer = 2, show elapsed time
			seconds_to_draw = (int)time;
		}
		
		int days, hours, minutes, seconds;
		unpack_seconds(seconds_to_draw, days, hours, minutes, seconds);

		if (hours > 0)
			sprintf(str, "%dh %dm %ds", hours, minutes, seconds);
		else if (minutes > 0)
			sprintf(str, "%d:%02d", minutes, seconds);
		else
			sprintf(str, "%d", seconds_to_draw);

		int r, g, b;
		UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
        if (hud_timer_height->value > 0.0f) {
            y = hud_timer_height->value;
		}
		gHUD.DrawHudStringCentered(x, y, str, r, g, b);
		return 1;
	}

	float timeleft = m_flSynced ? (m_flEndTime - time) : (m_flEndTime - m_flEffectiveTime);
	
	if (timeleft <= 0 && hud_timer->value == 1.0f) // Don't show negative time left
		return 0;
		
	char str[64];
	int seconds_to_draw;
	
	if (hud_timer->value == 1.0f) // time left
		seconds_to_draw = (int)(timeleft + 0.5f);
	else if (hud_timer->value == 2.0f) // time passed
		seconds_to_draw = (int)(time + 0.5f);
	else
		return 0;

	int days, hours, minutes, seconds;
	unpack_seconds(abs(seconds_to_draw), days, hours, minutes, seconds);

	if (days > 0)
		sprintf(str, "%s%d day%s %dh %dm %ds", (seconds_to_draw < 0 ? "-" : ""), days, (days > 1 ? "s" : ""), hours, minutes, seconds);
	else if (hours > 0)
		sprintf(str, "%s%dh %dm %ds", (seconds_to_draw < 0 ? "-" : ""), hours, minutes, seconds);
	else if (minutes > 0)
		sprintf(str, "%s%d:%02d", (seconds_to_draw < 0 ? "-" : ""), minutes, seconds);
	else
		sprintf(str, "%d", seconds_to_draw);

	int r, g, b;
	// Red color for low time (less than 60 seconds)
	if (hud_timer->value == 1.0f && seconds_to_draw <= 60 && seconds_to_draw > 0)
	{
		r = 255; g = 16; b = 16;
	}
	else
	{
		UnpackRGB(r, g, b, gHUD.m_iDefaultHUDColor);
	}
	
    if (hud_timer_height->value > 0.0f) {
        y = hud_timer_height->value;
    } else {
        y = gHUD.m_scrinfo.iCharHeight; // Default position if no custom height is set
	}
	gHUD.DrawHudStringCentered(x, y, str, r, g, b);

	return 1;
}

int CHudTimer::MsgFunc_Timer(const char* name, int size, void* buf)
{
	BEGIN_READ(buf, size);
	int timelimit = READ_LONG();
	int effectiveTime = READ_LONG();

	// Update message-based timer data
	seconds_total = timelimit;
	seconds_passed = effectiveTime;
	draw_until = gHUD.m_flTime + 5.0f;
	m_iFlags |= HUD_ACTIVE;

	// Also update sync data if not already synced
	if (!m_flSynced)
	{
		m_flEndTime = timelimit;
		m_flEffectiveTime = effectiveTime;
	}
	return 1;
}

void CHudTimer::Think()
{
    float flTime = gEngfuncs.GetClientTime();

    if (m_flNextSyncTime - flTime > 60.0f) 
        m_flNextSyncTime = flTime; 

    if (m_pCvarHudTimerSync != nullptr && m_pCvarHudTimerSync->value > 0.0f && m_flNextSyncTime <= flTime)
        SyncTimer(flTime);
}

void CHudTimer::SyncTimer(float fTime)
{

    if (m_pCvarHudTimerSync == nullptr || m_pCvarHudTimerSync->value == 0.0f) 
    {
        m_flSynced = false;
        return;
    }

    if (NET_API) 
    {
        NET_API->InitNetworking();

        net_status_t status;
        NET_API->Status(&status); 

        if (status.connected)
        {
            if (status.remote_address.type == NA_IP) 
            {
                SyncTimerRemote(*(unsigned int *)status.remote_address.ip, status.remote_address.port, fTime, status.latency);
                if (g_eRulesRequestStatus == SOCKET_AWAITING_CODE || g_eRulesRequestStatus == SOCKET_AWAITING_ANSWER)
                {
                    return; 
                }
            }
            else if (status.remote_address.type == NA_LOOPBACK)
            {
                if (m_pCvarMpTimelimit && m_pCvarMpTimeleft)
                {
                    m_flEndTime = m_pCvarMpTimelimit->value * 60.0f; 

                    if (!m_bDelayTimeleftReading)
                    {
                        float timeleft_cvar = m_pCvarMpTimeleft->value;
                        if (timeleft_cvar > 0)
                        {
                            float endtime_calculated_from_timeleft = timeleft_cvar + fTime;

                            if (fabs(m_flEndTime - endtime_calculated_from_timeleft) > 1.5f || m_flEndTime == 0)
                            {
                                m_flEndTime = endtime_calculated_from_timeleft;
                            }
                            m_flSynced = true; 
                        }
                        else if (m_flEndTime > 0) {
                        }
                    }
                }
                m_flNextSyncTime = fTime + 5.0f; 
            }
            else 
            {
                m_flSynced = false; 
                m_flNextSyncTime = fTime + 1.0f; 
            }

            if (m_bDelayTimeleftReading)
            {
                m_bDelayTimeleftReading = false;
                m_flNextSyncTime = fTime + 1.5f;
            }
        }
        else 
        {
            m_flSynced = false;
            if (g_timerSocket != 0)
            {
                NetCloseSocket(g_timerSocket);
                g_timerSocket = 0;
                g_eRulesRequestStatus = SOCKET_NONE;
            }
            m_flNextSyncTime = fTime + 1.0f; 
        }
    }
    else 
    {
        m_flSynced = false;
        m_flNextSyncTime = fTime + 5.0f; 
    }
}

void CHudTimer::SyncTimerRemote(unsigned int ip, unsigned short port, float fTime, double latency)
{
    char buffer[2048]; 
    int len = 0;

    if (fTime - m_flNextSyncTime > 3.0f && (g_eRulesRequestStatus == SOCKET_AWAITING_CODE || g_eRulesRequestStatus == SOCKET_AWAITING_ANSWER))
    {
        g_eRulesRequestStatus = SOCKET_IDLE; 
        NetCloseSocket(g_timerSocket); 
        g_timerSocket = 0;
    }

    switch (g_eRulesRequestStatus)
    {
    case SOCKET_NONE: 
    case SOCKET_IDLE: 
        m_iResponceID = 0;
        m_iReceivedSize = 0;
        m_iReceivedPackets = 0;
        m_iReceivedPacketsCount = 0;
        memset(m_szPacketBuffer, 0, sizeof(m_szPacketBuffer));

        if (g_timerSocket != 0) { 
             NetClearSocket(g_timerSocket); 
        }
        NetSendUdp(ip, port, "\xFF\xFF\xFF\xFFV\xFF\xFF\xFF\xFF", 9, &g_timerSocket);
        g_eRulesRequestStatus = SOCKET_AWAITING_CODE;
        m_flNextSyncTime = fTime; 
        return;

    case SOCKET_AWAITING_CODE: 
        len = NetReceiveUdp(ip, port, buffer, sizeof(buffer), g_timerSocket);
        if (len < 5) 
            return;


        if (*(int *)buffer == -1 && buffer[4] == 'A' && len == 9)
        {
            buffer[4] = 'V'; 
            NetSendUdp(ip, port, buffer, 9, &g_timerSocket);


            g_eRulesRequestStatus = SOCKET_AWAITING_ANSWER;
            m_flNextSyncTime = fTime; 
            return;
        }
        g_eRulesRequestStatus = SOCKET_AWAITING_ANSWER;
        break; 

    case SOCKET_AWAITING_ANSWER: 
        len = NetReceiveUdp(ip, port, buffer, sizeof(buffer), g_timerSocket);
        if (len < 5) 
            return;
        break; 
    }


    if (*(int *)buffer == -2 ) 
    {
        if (len < 9) return; 

        int requestID = *(int *)(buffer + 4);
        unsigned char headerInfo = buffer[8];
        int currentPacket = headerInfo >> 4;
        int totalPackets = headerInfo & 0x0F; 

        if (currentPacket >= totalPackets) return;

        if (m_iReceivedPacketsCount == 0) 
        {
            m_iResponceID = requestID;
        }
        else if (m_iResponceID != requestID)
        {
            return;
        }

        if (m_iReceivedPackets & (1 << currentPacket)) return; 

        int dataOffset = (1400 - 9) * currentPacket; 
        if (dataOffset + (len - 9) > sizeof(m_szPacketBuffer)) return; 

        memcpy(m_szPacketBuffer + dataOffset, buffer + 9, len - 9);
        m_iReceivedSize += (len - 9);
        m_iReceivedPackets |= (1 << currentPacket);
        m_iReceivedPacketsCount++;

        if (m_iReceivedPacketsCount < totalPackets) return; 
    }
    else if (*(int *)buffer == -1 && buffer[4] == 'E') 
    {
        if (len > sizeof(m_szPacketBuffer)) return; 
        memcpy(m_szPacketBuffer, buffer, len); 
        m_iReceivedSize = len; 
    }
    else
    {
        g_eRulesRequestStatus = SOCKET_IDLE;
        NetCloseSocket(g_timerSocket);
        g_timerSocket = 0;
        return;
    }

    if (*(int *)m_szPacketBuffer != -1 || m_szPacketBuffer[4] != 'E')
    {
        g_eRulesRequestStatus = SOCKET_IDLE;
         NetCloseSocket(g_timerSocket); 
         g_timerSocket = 0;
        return;
    }

    m_flSynced = true;
    g_eRulesRequestStatus = SOCKET_IDLE; 
    m_flNextSyncTime = fTime + 10.0f; 

    char *value;

    value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "mp_timelimit");
    if (value && value[0])
    {
        m_flEndTime = atof(value) * 60; 
    }
    else
    {
        m_flEndTime = 0; 
    }

    if (!m_bDelayTimeleftReading) 
    {
        value = NetGetRuleValueFromBuffer(m_szPacketBuffer, m_iReceivedSize, "mp_timeleft");
        if (value && value[0])
        {
            float timeleft_from_server = atof(value);
            if (timeleft_from_server > 0)
            {
                float calculated_endtime = timeleft_from_server + (fTime - latency);

                if (fabs(m_flEndTime - calculated_endtime) > 1.5 || m_flEndTime == 0)
                {
                    m_flEndTime = calculated_endtime;
                }
            }
        }
    }
	
    m_iReceivedSize = 0;
    m_iReceivedPackets = 0;
    m_iReceivedPacketsCount = 0;
    m_iResponceID = 0;
}

void CHudTimer::DoResync()
{
	m_bDelayTimeleftReading = true;
	m_flNextSyncTime = 0;
	m_flSynced = false;
}
