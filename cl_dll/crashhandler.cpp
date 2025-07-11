#include "crashhandler.h"

#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Psapi.lib")

void WriteRegisters(FILE* logFile, CONTEXT* ctx) {
    fprintf(logFile, "\nRegisters:\n");
    fprintf(logFile, "EAX = 0x%08X\n", ctx->Eax);
    fprintf(logFile, "EBX = 0x%08X\n", ctx->Ebx);
    fprintf(logFile, "ECX = 0x%08X\n", ctx->Ecx);
    fprintf(logFile, "EDX = 0x%08X\n", ctx->Edx);
    fprintf(logFile, "ESI = 0x%08X\n", ctx->Esi);
    fprintf(logFile, "EDI = 0x%08X\n", ctx->Edi);
    fprintf(logFile, "EBP = 0x%08X\n", ctx->Ebp);
    fprintf(logFile, "ESP = 0x%08X\n", ctx->Esp);
    fprintf(logFile, "EIP = 0x%08X\n", ctx->Eip);
    fprintf(logFile, "EFlags = 0x%08X\n", ctx->EFlags);
}

void WriteLoadedModules(FILE* logFile) {
    fprintf(logFile, "\nLoaded Modules:\n");
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileName(hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                fprintf(logFile, "%ls\n", szModName);
            }
        }
    }
}

void CreateCrashLog(EXCEPTION_POINTERS* pep, const char* dmpFilename) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char logName[MAX_PATH];
    sprintf(logName, "crash_%04d%02d%02d_%02d%02d%02d.txt",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    FILE* logFile = fopen(logName, "w");
    if (!logFile) return;

    fprintf(logFile, "Crash Report\n");
    fprintf(logFile, "============\n");
    fprintf(logFile, "Time: %04d-%02d-%02d %02d:%02d:%02d\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    if (pep && pep->ExceptionRecord) {
        fprintf(logFile, "Exception Code: 0x%08X\n", pep->ExceptionRecord->ExceptionCode);
        fprintf(logFile, "Exception Address: 0x%p\n", pep->ExceptionRecord->ExceptionAddress);
        fprintf(logFile, "Number of Parameters: %lu\n", pep->ExceptionRecord->NumberParameters);
    }

    fprintf(logFile, "Process ID: %lu\n", GetCurrentProcessId());
    fprintf(logFile, "Thread ID: %lu\n", GetCurrentThreadId());
    fprintf(logFile, "Dump file: %s\n", dmpFilename);

    if (pep && pep->ContextRecord) {
        WriteRegisters(logFile, pep->ContextRecord);
    }

    WriteLoadedModules(logFile);

    fclose(logFile);
}

void CreateMiniDump(EXCEPTION_POINTERS* pep) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char filename[MAX_PATH];
    sprintf(filename, "crash_%04d%02d%02d_%02d%02d%02d.dmp",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpWithFullMemory,
            pep ? &mdei : nullptr,
            nullptr,
            nullptr);

        CloseHandle(hFile);
    }

    CreateCrashLog(pep, filename);
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    CreateMiniDump(ExceptionInfo);
    return EXCEPTION_EXECUTE_HANDLER;
}

void InstallCrashHandler() {
    SetUnhandledExceptionFilter(CrashHandler);
}
