#pragma once

#include <Arduino.h>

// Application configuration structure - all persistent settings
struct AppConfig {
    char bot_token[100];
    char chat_id[15];
    char pc_mac[18];
    char pc_hostname[30];

    // Constructor to initialize with default empty values
    AppConfig() {
        bot_token[0] = '\0';
        chat_id[0] = '\0';
        pc_mac[0] = '\0';
        pc_hostname[0] = '\0';
    }
};

// ConfigManager: handles LittleFS persistence for AppConfig
class ConfigManager {
public:
    // Load configuration from LittleFS (/config.json)
    // Returns true on success, false on failure
    static bool load(AppConfig& config);

    // Save configuration to LittleFS (/config.json)
    // Returns true on success, false on failure
    static bool save(const AppConfig& config);

private:
    static const char* CONFIG_PATH;
};