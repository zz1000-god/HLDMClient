#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "joinlast.h"

// Cvar для збереження останнього IP сервера
cvar_t* cl_lastserver = nullptr;

// Додаємо змінні для контролю одноразового збереження
static char g_lastSavedIP[256] = "";
static bool g_alreadySaved = false;

// Зберігаємо оригінальну функцію Con_Printf
static void (*original_Con_Printf)(const char* fmt, ...) = nullptr;

class CJoinLastCommand
{
public:
    static void Init();
    static void SaveLastServer(char* serverIP);
    static void JoinLastCommand();
    static void OnServerConnect(char* serverIP);
    static void ParseConsoleMessage(const char* message);

private:
    static const char* GetLastServerIP();
    static void ExtractIPFromMessage(const char* message, char* outIP, int maxLen);
};

// Наша функція для перехоплення консольних повідомлень
void HookedConPrintf(const char* fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Парсимо повідомлення для пошуку "Connection accepted by"
    CJoinLastCommand::ParseConsoleMessage(buffer);

    // Викликаємо оригінальну функцію
    if (original_Con_Printf)
    {
        original_Con_Printf("%s", buffer);
    }
    else
    {
        // Якщо не вдалося зберегти оригінальну, використовуємо стандартну
        gEngfuncs.Con_Printf("%s", buffer);
    }
}

// Ініціалізація команди та cvar
void CJoinLastCommand::Init()
{
    // Реєструємо cvar для збереження IP
    cl_lastserver = gEngfuncs.pfnRegisterVariable("cl_lastserver", "", FCVAR_ARCHIVE);

    // Реєструємо наші команди
    gEngfuncs.pfnAddCommand("joinlast", JoinLastCommand);
    gEngfuncs.pfnAddCommand("jl", JoinLastCommand);

    gEngfuncs.Con_Printf("JoinLast system initialized. Use 'joinlast' or 'jl' to reconnect.\n");
}

// Парсинг консольних повідомлень
void CJoinLastCommand::ParseConsoleMessage(const char* message)
{
    if (!message)
        return;

    // Шукаємо повідомлення "Connection accepted by"
    const char* connectionMsg = strstr(message, "Connection accepted by");
    if (connectionMsg)
    {
        char serverIP[256];
        ExtractIPFromMessage(connectionMsg, serverIP, sizeof(serverIP));
        
        if (strlen(serverIP) > 0)
        {
            SaveLastServer(serverIP);
        }
    }
}

// Витягування IP з повідомлення
void CJoinLastCommand::ExtractIPFromMessage(const char* message, char* outIP, int maxLen)
{
    if (!message || !outIP)
        return;

    // Очищуємо вихідний буфер
    outIP[0] = '\0';

    // Шукаємо початок IP після "Connection accepted by "
    const char* ipStart = strstr(message, "Connection accepted by ");
    if (!ipStart)
        return;

    ipStart += strlen("Connection accepted by ");

    // Пропускаємо пробіли
    while (*ipStart == ' ')
        ipStart++;

    // Копіюємо IP до першого пробілу, нового рядка або кінця рядка
    int i = 0;
    while (i < maxLen - 1 && *ipStart && *ipStart != ' ' && *ipStart != '\n' && *ipStart != '\r')
    {
        outIP[i] = *ipStart;
        ipStart++;
        i++;
    }
    outIP[i] = '\0';

    // Видаляємо зайві символи в кінці (якщо є)
    while (i > 0 && (outIP[i-1] == '.' || outIP[i-1] == ' '))
    {
        i--;
        outIP[i] = '\0';
    }
}

// Збереження IP останнього сервера (тільки один раз)
void CJoinLastCommand::SaveLastServer(char* serverIP)
{
    if (!serverIP || strlen(serverIP) == 0 || !cl_lastserver)
        return;

    // Перевіряємо, чи це не localhost або локальна адреса
    if (strstr(serverIP, "127.0.0.1") || strstr(serverIP, "localhost"))
        return;

    // ДОДАЄМО ПЕРЕВІРКУ: зберігаємо тільки якщо IP змінився
    if (g_alreadySaved && strcmp(g_lastSavedIP, serverIP) == 0)
    {
        // Вже збережено цей IP, не зберігаємо знову
        return;
    }

    // Зберігаємо IP в cvar
    gEngfuncs.Cvar_Set("cl_lastserver", serverIP);

    // ОНОВЛЮЄМО контрольні змінні
    strcpy(g_lastSavedIP, serverIP);
    g_alreadySaved = true;

    gEngfuncs.Con_Printf("Last server saved: %s\n", serverIP);
}

// Виконання команди joinlast
void CJoinLastCommand::JoinLastCommand()
{
    const char* lastIP = GetLastServerIP();

    if (!lastIP || strlen(lastIP) == 0)
    {
        gEngfuncs.Con_Printf("No previous server found. Connect to a server first.\n");
        gEngfuncs.Con_Printf("Use: connect <ip:port>\n");
        return;
    }

    gEngfuncs.Con_Printf("Connecting to last server: %s\n", lastIP);

    // СКИДАЄМО флаг для нового підключення
    g_alreadySaved = false;
    strcpy(g_lastSavedIP, "");

    // Формуємо команду підключення
    char connectCmd[256];
    snprintf(connectCmd, sizeof(connectCmd), "connect %s", lastIP);

    // Виконуємо команду підключення
    gEngfuncs.pfnClientCmd(connectCmd);
}

// Отримання IP останнього сервера з cvar
const char* CJoinLastCommand::GetLastServerIP()
{
    if (!cl_lastserver)
        return nullptr;

    return cl_lastserver->string;
}

// Функція для виклику при підключенні до сервера
void CJoinLastCommand::OnServerConnect(char* serverIP)
{
    if (serverIP && strlen(serverIP) > 0)
    {
        SaveLastServer(serverIP);
    }
}

// Додаткова команда для перегляду збереженого IP
void ShowLastServerCommand()
{
    const char* lastIP = cl_lastserver ? cl_lastserver->string : "";

    if (strlen(lastIP) == 0)
    {
        gEngfuncs.Con_Printf("No server IP saved.\n");
    }
    else
    {
        gEngfuncs.Con_Printf("Last server: %s\n", lastIP);
    }
}

// Команда для очищення збереженого IP
void ClearLastServerCommand()
{
    if (cl_lastserver)
    {
        gEngfuncs.Cvar_Set("cl_lastserver", "");
        gEngfuncs.Con_Printf("Last server IP cleared.\n");
    }
    
    // СКИДАЄМО контрольні змінні
    strcpy(g_lastSavedIP, "");
    g_alreadySaved = false;
}

// Команда для тестування парсингу
void TestParseCommand()
{
    // Тестове повідомлення
    const char* testMsg = "Connection accepted by 192.168.1.100:27015";
    gEngfuncs.Con_Printf("Testing parse with: %s\n", testMsg);
    CJoinLastCommand::ParseConsoleMessage(testMsg);
}

// Глобальні функції для використання в інших частинах коду
extern "C"
{
    void JoinLast_Init()
    {
        CJoinLastCommand::Init();

        // Додаткові команди
        gEngfuncs.pfnAddCommand("lastserver", ShowLastServerCommand);
        gEngfuncs.pfnAddCommand("clearlast", ClearLastServerCommand);
        gEngfuncs.pfnAddCommand("testparse", TestParseCommand);
    }

    void JoinLast_OnConnect(char* serverIP)
    {
        CJoinLastCommand::OnServerConnect(serverIP);
    }

    // Функція для обробки консольних повідомлень (викликати з hud.cpp)
    void JoinLast_ParseConsoleMessage(const char* message)
    {
        CJoinLastCommand::ParseConsoleMessage(message);
    }

    // Функція для отримання поточного IP сервера
    const char* JoinLast_GetCurrentServerIP()
    {
        if (cl_lastserver)
            return cl_lastserver->string;
        return nullptr;
    }
}