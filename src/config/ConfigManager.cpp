#include "ConfigManager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../utils/Logger.h"

// Definition of static member
const char* ConfigManager::CONFIG_PATH = "/config.json";

bool ConfigManager::load(AppConfig& config) {
    if (!LittleFS.exists(CONFIG_PATH)) {
        LOG_W("ConfigMgr", "No config file found, using defaults");
        return false;
    }

    File configFile = LittleFS.open(CONFIG_PATH, "r");
    if (!configFile) {
        LOG_E("ConfigMgr", "Failed to open config file for reading");
        return false;
    }

    JsonDocument json;
    DeserializationError error = deserializeJson(json, configFile);
    configFile.close();

    if (error) {
        LOG_E("ConfigMgr", String("JSON deserialization failed: ") + error.c_str());
        return false;
    }

    // Copy values from JSON to AppConfig struct
    if (json.containsKey("bot_token")) {
        strncpy(config.bot_token, json["bot_token"].as<const char*>(), sizeof(config.bot_token) - 1);
    }
    if (json.containsKey("chat_id")) {
        strncpy(config.chat_id, json["chat_id"].as<const char*>(), sizeof(config.chat_id) - 1);
    }
    if (json.containsKey("pc_mac")) {
        strncpy(config.pc_mac, json["pc_mac"].as<const char*>(), sizeof(config.pc_mac) - 1);
    }
    if (json.containsKey("pc_hostname")) {
        strncpy(config.pc_hostname, json["pc_hostname"].as<const char*>(), sizeof(config.pc_hostname) - 1);
    }

    LOG_I("ConfigMgr", "Configuration loaded successfully");
    return true;
}

bool ConfigManager::save(const AppConfig& config) {
    JsonDocument json;
    json["bot_token"] = config.bot_token;
    json["chat_id"] = config.chat_id;
    json["pc_mac"] = config.pc_mac;
    json["pc_hostname"] = config.pc_hostname;

    File configFile = LittleFS.open(CONFIG_PATH, "w");
    if (!configFile) {
        LOG_E("ConfigMgr", "Failed to open config file for writing");
        return false;
    }

    serializeJson(json, configFile);
    configFile.close();

    LOG_I("ConfigMgr", "Configuration saved successfully");
    return true;
}