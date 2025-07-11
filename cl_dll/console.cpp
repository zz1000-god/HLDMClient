#include "hud.h"
#include "cl_util.h"
#include "console.h"
#include <algorithm>
#include <cstdio>

ConsoleHook* ConsoleHook::instance = nullptr;

// Predefined colors
const ConsoleColor ConsoleHook::Red(249, 54, 54, 255);
const ConsoleColor ConsoleHook::Green(77, 219, 83, 255);
const ConsoleColor ConsoleHook::Yellow(240, 205, 65, 255);
const ConsoleColor ConsoleHook::Cyan(111, 234, 247, 255);
const ConsoleColor ConsoleHook::White(255, 255, 255, 255);
const ConsoleColor ConsoleHook::Orange(255, 165, 0, 255);
const ConsoleColor ConsoleHook::Blue(0, 100, 255, 255);
const ConsoleColor ConsoleHook::Purple(128, 0, 128, 255);

ConsoleHook::ConsoleHook()
    : initialized(false), gameUILoaded(false), consoleReady(false),
    originalConPrintf(nullptr), originalConDPrintf(nullptr),
    defaultColor(216, 222, 211, 255), defaultDColor(196, 181, 80, 255),
    currentConsoleColor(nullptr), currentConsoleDColor(nullptr),
    useColorCodes(true) { // Увімкнути кольорові коди за замовчуванням
}

void ConsoleHook::SetConsoleColor(ConsoleColor color) {
    if (useColorCodes) {
        currentColorCode = GetColorCode(color);
    }

    // Для Half-Life engine можна використовувати Con_Printf з кольоровими кодами
    if (currentConsoleColor) {
        *currentConsoleColor = color;
    }
    if (currentConsoleDColor) {
        *currentConsoleDColor = color;
    }
}

std::string ConsoleHook::GetColorCode(ConsoleColor color) {
    // Half-Life використовує ^1, ^2, ^3 тощо для кольорів (як у Quake)
    // Або можна використовувати ANSI escape коди

    // Варіант 1: Half-Life color codes (якщо підтримуються)
    if (color.r == 249 && color.g == 54 && color.b == 54) return "^1"; // Red
    if (color.r == 77 && color.g == 219 && color.b == 83) return "^2"; // Green  
    if (color.r == 240 && color.g == 205 && color.b == 65) return "^3"; // Yellow
    if (color.r == 111 && color.g == 234 && color.b == 247) return "^4"; // Cyan
    if (color.r == 255 && color.g == 255 && color.b == 255) return "^7"; // White
    if (color.r == 255 && color.g == 165 && color.b == 0) return "^6"; // Orange
    if (color.r == 0 && color.g == 100 && color.b == 255) return "^5"; // Blue
    if (color.r == 128 && color.g == 0 && color.b == 128) return "^8"; // Purple

    // Варіант 2: ANSI escape коди (якщо консоль підтримує)
    /*
    if (color.r == 249 && color.g == 54 && color.b == 54) return "\033[31m"; // Red
    if (color.r == 77 && color.g == 219 && color.b == 83) return "\033[32m"; // Green
    if (color.r == 240 && color.g == 205 && color.b == 65) return "\033[33m"; // Yellow
    if (color.r == 111 && color.g == 234 && color.b == 247) return "\033[36m"; // Cyan
    if (color.r == 255 && color.g == 255 && color.b == 255) return "\033[37m"; // White
    if (color.r == 255 && color.g == 165 && color.b == 0) return "\033[35m"; // Orange/Magenta
    if (color.r == 0 && color.g == 100 && color.b == 255) return "\033[34m"; // Blue
    if (color.r == 128 && color.g == 0 && color.b == 128) return "\033[35m"; // Purple
    */

    return "^7"; // Default white
}

void ConsoleHook::EnableColorCodes(bool enable) {
    useColorCodes = enable;
}

ConsoleHook::~ConsoleHook() {
    HudShutdown();
}

ConsoleHook* ConsoleHook::GetInstance() {
    if (!instance) {
        instance = new ConsoleHook();
    }
    return instance;
}

bool ConsoleHook::Initialize() {
    if (initialized) {
        return true;
    }

    originalConPrintf = gEngfuncs.Con_Printf;
    originalConDPrintf = gEngfuncs.Con_DPrintf;

    gEngfuncs.Con_Printf = (pfnEngSrc_Con_Printf_t)HookedConPrintf;
    gEngfuncs.Con_DPrintf = (pfnEngSrc_Con_DPrintf_t)HookedConDPrintf;

    initialized = true;
    return true;
}

void ConsoleHook::HudInit() {
    if (!initialized) return;

    // Check if in developer mode
    bool isDev = gEngfuncs.CheckParm("-dev", nullptr) != 0;

    if (isDev) {
        consoleReady = true;
        ProcessEarlyMessages();
    }

    // Attempt to hook GameConsole
    HookGameConsole();
}

void ConsoleHook::HudPostInit() {
    if (!initialized) return;

    if (!consoleReady) {
        // Console should be ready by now
        consoleReady = true;
        ProcessEarlyMessages();
    }
}

void ConsoleHook::HudShutdown() {
    if (!initialized) return;

    // Restore original functions
    if (originalConPrintf) {
        gEngfuncs.Con_Printf = originalConPrintf;
    }
    if (originalConDPrintf) {
        gEngfuncs.Con_DPrintf = originalConDPrintf;
    }

    // Clear custom lines
    ClearCustomLines();

    // Clear early messages queue
    while (!earlyMessages.empty()) {
        earlyMessages.pop();
    }

    consoleReady = false;
    gameUILoaded = false;
    initialized = false;
}

void ConsoleHook::Update(float deltaTime) {
    if (!initialized) return;

    UpdateCustomLines(deltaTime);
}

void ConsoleHook::Draw() {
    if (!initialized || !consoleReady) return;

    DrawCustomLines();
}

void ConsoleHook::HookedConPrintf(const char* fmt, ...) {
    ConsoleHook* hook = GetInstance();
    if (!hook || !hook->originalConPrintf) return;

    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    // Call original function
    hook->originalConPrintf("%s", buffer);

    va_end(args);
}

void ConsoleHook::HookedConDPrintf(const char* fmt, ...) {
    ConsoleHook* hook = GetInstance();
    if (!hook || !hook->originalConDPrintf) return;

    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    // Call original function
    hook->originalConDPrintf("%s", buffer);

    va_end(args);
}

void ConsoleHook::ProcessEarlyMessages() {
    while (!earlyMessages.empty()) {
        EarlyConsoleMessage& msg = earlyMessages.front();

        SetConsoleColor(msg.color);
        SafeConPrintf("%s", msg.text.c_str());
        ResetConsoleColor();

        earlyMessages.pop();
    }
}

void ConsoleHook::UpdateCustomLines(float deltaTime) {
    for (auto it = customLines.begin(); it != customLines.end();) {
        it->timeLeft -= deltaTime;

        if (it->fadeOut && it->timeLeft <= 1.0f) {
            // Calculate alpha for fade out effect
            it->alpha = it->timeLeft;
        }

        if (it->timeLeft <= 0.0f) {
            it = customLines.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ConsoleHook::DrawCustomLines() {
    // For Half-Life, custom drawing would use HUD functions
    // This is a placeholder for actual HUD drawing implementation

    for (const auto& line : customLines) {
        if (!line.visible) continue;

        // Here you would use Half-Life's HUD drawing functions
        // Example: DrawHudString(line.x, line.y, line.text.c_str(), line.color.r, line.color.g, line.color.b);
    }
}

bool ConsoleHook::HookGameConsole() {
    // Placeholder for GameConsole hooking (if needed)
    // This depends on specific Half-Life implementation
    gameUILoaded = true;
    SetupConsoleColors();
    return true;
}

void ConsoleHook::SetupConsoleColors() {
    // Setup console colors and pointers
    // This would depend on specific Half-Life console implementation
}

int ConsoleHook::AddCustomLine(const std::string& text, int x, int y, ConsoleColor color, float duration, bool fadeOut) {
    ConsoleLineInfo newLine(text, x, y, color, duration, fadeOut);
    customLines.push_back(newLine);
    return customLines.size() - 1;
}

void ConsoleHook::RemoveCustomLine(int index) {
    if (index >= 0 && index < customLines.size()) {
        customLines.erase(customLines.begin() + index);
    }
}

void ConsoleHook::UpdateCustomLine(int index, const std::string& newText) {
    if (index >= 0 && index < customLines.size()) {
        customLines[index].text = newText;
    }
}

void ConsoleHook::SetCustomLineColor(int index, ConsoleColor color) {
    if (index >= 0 && index < customLines.size()) {
        customLines[index].color = color;
    }
}

void ConsoleHook::SetCustomLinePosition(int index, int x, int y) {
    if (index >= 0 && index < customLines.size()) {
        customLines[index].x = x;
        customLines[index].y = y;
    }
}

void ConsoleHook::ClearCustomLines() {
    customLines.clear();
}

void ConsoleHook::PrintColored(ConsoleColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (consoleReady) {
        if (useColorCodes) {
            std::string colorCode = GetColorCode(color);
            std::string resetCode = GetColorCode(White); // Reset to white
            
            // Форматуємо повідомлення з кольоровими кодами
            char coloredBuffer[1200];
            snprintf(coloredBuffer, sizeof(coloredBuffer), "%s%s%s", 
                    colorCode.c_str(), buffer, resetCode.c_str());
            
            SafeConPrintf("%s", coloredBuffer);
        } else {
            SetConsoleColor(color);
            SafeConPrintf("%s", buffer);
            ResetConsoleColor();
        }
    }
    else {
        earlyMessages.push(EarlyConsoleMessage(buffer, color, 0.0f));
    }

    va_end(args);
}

void ConsoleHook::PrintColoredDev(ConsoleColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (consoleReady) {
        if (useColorCodes) {
            std::string colorCode = GetColorCode(color);
            std::string resetCode = GetColorCode(White);
            
            char coloredBuffer[1200];
            snprintf(coloredBuffer, sizeof(coloredBuffer), "%s%s%s", 
                    colorCode.c_str(), buffer, resetCode.c_str());
            
            SafeConDPrintf("%s", coloredBuffer);
        } else {
            SetConsoleColor(color);
            SafeConDPrintf("%s", buffer);
            ResetConsoleColor();
        }
    }
    else {
        if (gEngfuncs.CheckParm("-dev", nullptr)) {
            earlyMessages.push(EarlyConsoleMessage(buffer, color, 0.0f));
        }
    }

    va_end(args);
}

void ConsoleHook::Print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (consoleReady) {
        SafeConPrintf("%s", buffer);
    }
    else {
        earlyMessages.push(EarlyConsoleMessage(buffer, defaultColor, 0.0f));
    }

    va_end(args);
}

void ConsoleHook::PrintDev(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (consoleReady) {
        SafeConDPrintf("%s", buffer);
    }
    else {
        if (gEngfuncs.CheckParm("-dev", nullptr)) {
            earlyMessages.push(EarlyConsoleMessage(buffer, defaultDColor, 0.0f));
        }
    }

    va_end(args);
}

void ConsoleHook::SetConsoleColor(ConsoleColor color) {
    if (useColorCodes) {
        currentColorCode = GetColorCode(color);
    }

    // Для Half-Life engine можна використовувати Con_Printf з кольоровими кодами
    if (currentConsoleColor) {
        *currentConsoleColor = color;
    }
    if (currentConsoleDColor) {
        *currentConsoleDColor = color;
    }
}

void ConsoleHook::ResetConsoleColor() {
    SetConsoleColor(defaultColor);
    if (currentConsoleDColor) {
        *currentConsoleDColor = defaultDColor;
    }
}

ConsoleColor ConsoleHook::GetConsoleColor() const {
    if (currentConsoleColor) {
        return *currentConsoleColor;
    }
    return defaultColor;
}

void ConsoleHook::SafeConPrintf(const char* fmt, ...) {
    if (!originalConPrintf) return;

    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    originalConPrintf("%s", buffer);

    va_end(args);
}

void ConsoleHook::SafeConDPrintf(const char* fmt, ...) {
    if (!originalConDPrintf) return;

    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    originalConDPrintf("%s", buffer);

    va_end(args);
}

// Usage examples:
/*
void InitializeConsoleHook() {
    ConsoleHook* hook = ConsoleHook::GetInstance();

    // Initialize in ClientDLL Initialize
    hook->Initialize();
}

void OnHudInit() {
    ConsoleHook* hook = ConsoleHook::GetInstance();
    hook->HudInit();
}

void OnHudPostInit() {
    ConsoleHook* hook = ConsoleHook::GetInstance();
    hook->HudPostInit();
}

void OnHudUpdate(float deltaTime) {
    ConsoleHook* hook = ConsoleHook::GetInstance();
    hook->Update(deltaTime);
}

void OnHudDraw() {
    ConsoleHook* hook = ConsoleHook::GetInstance();
    hook->Draw();
}

void OnHudShutdown() {
    ConsoleHook* hook = ConsoleHook::GetInstance();
    hook->HudShutdown();
}

void ExampleUsage() {
    ConsoleHook* hook = ConsoleHook::GetInstance();

    // Colored console output
    hook->PrintColored(ConsoleHook::Red, "Error: Critical system failure!\n");
    hook->PrintColored(ConsoleHook::Green, "Success: Mission completed!\n");
    hook->PrintColored(ConsoleHook::Yellow, "Warning: Low ammunition!\n");

    // Custom HUD lines
    hook->AddCustomLine("Player Health: 25", 10, 50, ConsoleHook::Red, 3.0f, true);
    hook->AddCustomLine("Ammunition: 120/240", 10, 70, ConsoleHook::White, 5.0f);
    hook->AddCustomLine("Status: Active", 10, 90, ConsoleHook::Green, 2.0f);
}
*/