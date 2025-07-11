#pragma once

#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

// Викликати цю функцію один раз при старті (наприклад, у GameDLLInit)
void InstallCrashHandler();
void CauseCrash();

#endif // CRASH_HANDLER_H
