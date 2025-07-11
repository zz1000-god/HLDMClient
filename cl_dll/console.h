#pragma once;

#include <windows.h>
#include <vector>
#include <string>
#include <cstdarg>
#include <queue>
#include "APIProxy.h"

// Forward declarations
struct ConsoleColor {
    unsigned char r, g, b, a;

    ConsoleColor() : r(255), g(255), b(255), a(255) {}
    ConsoleColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {
    }
};
class IGameConsole;

// ��������� ��� ��������� ���������� ��� �����
struct ConsoleLineInfo {
    std::string text;
    int x, y;
    ConsoleColor color;
    float duration;
    float timeLeft;
    bool visible;
    bool fadeOut;
    float alpha;

    ConsoleLineInfo(const std::string& txt, int posX, int posY, ConsoleColor col, float dur, bool fade = false)
        : text(txt), x(posX), y(posY), color(col), duration(dur), timeLeft(dur),
        visible(true), fadeOut(fade), alpha(1.0f) {
    }
};

// ��������� ��� ����� ���������� (�� ����������� ������)
struct EarlyConsoleMessage {
    std::string text;
    ConsoleColor color;
    float duration;

    EarlyConsoleMessage(const std::string& txt, ConsoleColor col, float dur)
        : text(txt), color(col), duration(dur) {
    }
};

class ConsoleHook {
private:
    static ConsoleHook* instance;

    // �������� ��� ��������� ����������
    std::vector<ConsoleLineInfo> customLines;
    std::queue<EarlyConsoleMessage> earlyMessages;

    // ���� �����������
    bool initialized;
    bool gameUILoaded;
    bool consoleReady;

    // ��������� ������� ������
    pfnEngSrc_Con_Printf_t originalConPrintf;
    pfnEngSrc_Con_DPrintf_t originalConDPrintf;

    // ���� ��� ������
    static void HookedConPrintf(const char* fmt, ...);
    static void HookedConDPrintf(const char* fmt, ...);

    // ������� �� �������������
    ConsoleColor defaultColor;
    ConsoleColor defaultDColor;

    // �������� ���� ������
    ConsoleColor* currentConsoleColor;
    ConsoleColor* currentConsoleDColor;

    // ������� ������
    void ProcessEarlyMessages();
    void UpdateCustomLines(float deltaTime);
    void DrawCustomLines();
    bool HookGameConsole();
    void SetupConsoleColors();

    // ������� ������
    void SafeConPrintf(const char* fmt, ...);
    void SafeConDPrintf(const char* fmt, ...);

    bool useColorCodes;
    std::string currentColorCode;

public:
    ConsoleHook();
    ~ConsoleHook();

    static ConsoleHook* GetInstance();

    // ������� ����
    bool Initialize();
    void HudInit();
    void HudPostInit();
    void HudShutdown();
    void Update(float deltaTime);
    void Draw();

    // API ��� ������ � ���������� �������
    int AddCustomLine(const std::string& text, int x, int y, ConsoleColor color, float duration = 5.0f, bool fadeOut = false);
    void RemoveCustomLine(int index);
    void UpdateCustomLine(int index, const std::string& newText);
    void SetCustomLineColor(int index, ConsoleColor color);
    void SetCustomLinePosition(int index, int x, int y);
    void ClearCustomLines();

    // API ��� ������ � ����������� �������������
    void PrintColored(ConsoleColor color, const char* fmt, ...);
    void PrintColoredDev(ConsoleColor color, const char* fmt, ...);
    void Print(const char* fmt, ...);
    void PrintDev(const char* fmt, ...);

    // ��������� ��������� ������
    void SetConsoleColor(ConsoleColor color);
    void ResetConsoleColor();
    ConsoleColor GetConsoleColor() const;

    // ������
    bool IsInitialized() const { return initialized; }
    bool IsConsoleReady() const { return consoleReady; }
    int GetCustomLineCount() const { return customLines.size(); }

    // ������� ������� ��� ��������
    static const ConsoleColor Red;
    static const ConsoleColor Green;
    static const ConsoleColor Yellow;
    static const ConsoleColor Cyan;
    static const ConsoleColor White;
    static const ConsoleColor Orange;
    static const ConsoleColor Blue;
    static const ConsoleColor Purple;

    void EnableColorCodes(bool enable = true);
    std::string GetColorCode(ConsoleColor color);

};