#include "config_fs.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Le avisamos que estas variables globales viven en main.cpp
extern char bot_token[100];
extern char chat_id[15];

void saveConfigFile() {
  JsonDocument json;
  json["bot_token"] = bot_token;
  json["chat_id"] = chat_id;
  json["umbral"] = umbral;
  
  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(json, configFile);
    configFile.close();
    Serial.println("✅ Archivo guardado correctamente en LittleFS!");
  } else {
    Serial.println("❌ ERROR FATAL: No se pudo crear config.json");
  }
}

void loadConfigFile() {
  if (LittleFS.exists("/config.json")) {
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      JsonDocument json;
      DeserializationError error = deserializeJson(json, configFile);
      if (!error) {
        strcpy(bot_token, json["bot_token"]);
        strcpy(chat_id, json["chat_id"]);
        umbral = json["umbral"] | 50.0f; //inicializa el umbral en 50°C si no hay datos por defecto
      }
      configFile.close();
    }
  }
}