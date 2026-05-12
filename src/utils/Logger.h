#pragma once

// Logger levels: 0 = NONE, 1 = ERROR, 2 = WARN, 3 = INFO
#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

// Color codes for terminal output (optional, can be disabled)
#define LOG_COLOR_NONE    "\033[0m"
#define LOG_COLOR_RED     "\033[0;31m"
#define LOG_COLOR_YELLOW  "\033[0;33m"
#define LOG_COLOR_GREEN   "\033[0;32m"

// LOG_LEVEL 0: Silent (no output)
#if LOG_LEVEL >= 1
#define LOG_E(tag, msg) Serial.println(String("[") + String(tag) + String("] ERROR: ") + String(msg))
#else
#define LOG_E(tag, msg) ((void)0)
#endif

#if LOG_LEVEL >= 2
#define LOG_W(tag, msg) Serial.println(String("[") + String(tag) + String("] WARN: ") + String(msg))
#else
#define LOG_W(tag, msg) ((void)0)
#endif

#if LOG_LEVEL >= 3
#define LOG_I(tag, msg) Serial.println(String("[") + String(tag) + String("] ") + String(msg))
#else
#define LOG_I(tag, msg) ((void)0)
#endif

// Optional: Debug macro for development (LOG_LEVEL 4)
#if LOG_LEVEL >= 4
#define LOG_D(tag, msg) Serial.println(String("[") + String(tag) + String("] DEBUG: ") + String(msg))
#else
#define LOG_D(tag, msg) ((void)0)
#endif