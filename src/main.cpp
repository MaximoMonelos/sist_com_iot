#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RMaker.h>
#include <WiFiProv.h>

#define PIN_RELE 26
#define PIN_FAN_CONTROL 25
char bot_token[100] = "";
char chat_id[15] = "";

// Objetos globales
WiFiClientSecure client;
UniversalTelegramBot *bot;

unsigned long lastTimeBotRan;
unsigned long ultimo_reporte_temp = 0;
const int boot_button = 0; // Botón físico de BOOT del ESP32
const int PIN_SENSOR_TEMP = 34; // Pin ADC donde vas a leer el 1N4148 (ejemplo)
 

// 1. Creamos un dispositivo dedicado a la configuración para no mezclarlo con tu hardware
Device config_device("Configuracion");

Param token_param("Bot Token", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
Param chat_id_param("Chat ID", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);

//Dispositivos estándar RainMaker
TemperatureSensor sensorTemp("Sensor Temp"); 
Switch actuadorFan("Ventilador");
Switch releLuz("Luz");
Switch motor("Motor Principal");
Param rpm_act("RPM_ACTUALES","esp,param.rpm_actuales",value(0), PROP_FLAG_READ);
Param rpm_set("RPM_SETPOINT","esp,param.rpm_setpoint",value(0), PROP_FLAG_READ | PROP_FLAG_WRITE);

// Dispositivo Custom (Motor) - Le mezclamos control y lectura


// --- MANEJO DE ARCHIVOS (LittleFS) ---
void saveConfigFile() {
  JsonDocument json;
  json["bot_token"] = bot_token;
  json["chat_id"] = chat_id;
  
  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(json, configFile);
    configFile.close();
    Serial.println("✅ Archivo guardado correctamente en LittleFS!");
  }
  else{
    Serial.println("❌ ERROR FATAL: No se pudo crear config.json");
    return;
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
      }
    }
  }
}

void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START: Serial.println("Provisioning (BLE) Iniciado. Abrí la app RainMaker!"); break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS: Serial.println("WiFi Correcto! Conectando..."); break;
    case ARDUINO_EVENT_PROV_END: Serial.println("Conexión finalizada exitosamente."); break;
    default: break;
  }
}

// --------- LÓGICA DE TELEGRAM -----------
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot->messages[i].text;
    String id = String(bot->messages[i].chat_id);
    
    if (id != chat_id){
      continue; // Ignora mensajes de gente que no seas vos
    }
    
    // Acá podés agregar comandos como "/estado", "/temp", "/motor"
    if (text == "/start") {
      String welcome = "Bot de Comunicaciones:\n/estado - Ver estado del sistema\n/reset_wifi (Cuidado)";
      bot->sendMessage(id, welcome, "");
    }
    
    if (text == "/estado") {
      // Ejemplo a futuro: leer variables del motor y temperatura y mandarlas
      bot->sendMessage(id, "El sistema está funcionando. (Acá irán los datos del motor/temp)", "");
    }
  }
}

// --------- CALLBACK DE RAINMAKER -----------
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();
  Serial.printf(">>> Callback disparado! Dispositivo: '%s' | Param: '%s'\n", device_name, param_name);
  // A. BLOQUE DE CONFIGURACIÓN (No tocar)
  if (strcmp(device_name, "Configuracion") == 0) { 
    
    if (strcmp(param_name, "Bot Token") == 0) {
      memset(bot_token, 0, sizeof(bot_token));
      strncpy(bot_token, val.val.s, sizeof(bot_token) - 1);
      param->updateAndReport(val);
      saveConfigFile();
      
      if (bot != nullptr) {
        delete bot;
        bot = nullptr;
      }
      bot = new UniversalTelegramBot(bot_token, client);
      Serial.println("Bot token guardado y bot recreado.");
    }
    
    else if (strcmp(param_name, "Chat ID") == 0) {
      memset(chat_id, 0, sizeof(chat_id));
      strncpy(chat_id, val.val.s, sizeof(chat_id) - 1);
      param->updateAndReport(val);
      saveConfigFile();
      Serial.println("Chat ID guardado.");
    }
  }
    // Ejemplos vacíos para los actuadores:
    else if (strcmp(device_name, "Ventilador") == 0) {
      if (strcmp(param_name, "Power") == 0) {
        bool estado = val.val.b;
        Serial.printf("RainMaker envió el estado: %d\n", estado);
        digitalWrite(PIN_FAN_CONTROL, estado);
        param->updateAndReport(val); 
      }
    }
    else if (strcmp(device_name, "Luz") == 0) {
      if (strcmp(param_name, "Power") == 0) {
        bool estado_luz = val.val.b;
        Serial.printf("RainMaker envió el estado: %d\n", estado_luz);
        digitalWrite(PIN_RELE,!estado_luz);
        param->updateAndReport(val);

      }
    }
    else if (strcmp(device_name, "Motor Principal") == 0) {
      if (strcmp(param_name, "Power") == 0) {
        // bool estado = val.val.b;
        // digitalWrite(PIN_MOTOR, estado);
        param->updateAndReport(val); 
      }
    }
}



void setup() {
  Serial.begin(115200);
  pinMode(boot_button, INPUT_PULLUP); // Botón físico de reset
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_FAN_CONTROL, OUTPUT);
  digitalWrite(PIN_RELE, HIGH);  //Luz Apagada por default
  digitalWrite(PIN_FAN_CONTROL, LOW); //Fan Apagado por default


  if (!LittleFS.begin(true)) { Serial.println("Error LittleFS"); return; }
  loadConfigFile(); 

  // --- CONFIGURACIÓN DEL NODO RAINMAKER ---
  Node my_node = RMaker.initNode("ESP32-Comunicaciones"); // Nombre de tu placa

  // ==========================================
  // 3. ARMADO DE LA JERARQUÍA RAINMAKER
  // ==========================================
  
  // A. Configurar Dispositivo Telegram
  config_device.addCb(write_callback);
  token_param.updateAndReport(esp_rmaker_str(bot_token));
  chat_id_param.updateAndReport(esp_rmaker_str(chat_id));
  config_device.addParam(token_param);
  config_device.addParam(chat_id_param);
  
  // B. Configurar Dispositivo Motor
  motor.addCb(write_callback);
  motor.addParam(rpm_act);
  motor.addParam(rpm_set);


  // C. Enganchar los callbacks de los switches estándar
  actuadorFan.addCb(write_callback);
  releLuz.addCb(write_callback);

  // D. Colgar todos los Dispositivos al Nodo Principal
  my_node.addDevice(config_device);
  my_node.addDevice(sensorTemp);
  my_node.addDevice(actuadorFan);
  my_node.addDevice(releLuz);
  my_node.addDevice(motor);

  // --- INICIO DE SERVICIOS ---
  RMaker.start(); // Prepara RainMaker
  WiFi.onEvent(sysProvEvent);

  WiFi.mode(WIFI_STA); 
  btStop();            
  delay(100);           
  
  // Nombre del Bluetooth para configurar ("NODO_COMMS") y PIN ("pop1234567")
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, "pop1234567", "NODO_COMMS");

  Serial.print("Esperando conexión WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi Conectado!");
  releLuz.updateAndReportParam("Power", false);
  actuadorFan.updateAndReportParam("Power", false);
  // Preparamos Telegram
  client.setInsecure();
  bot = new UniversalTelegramBot(bot_token, client);
}

void loop() {
  // Lógica de reseteo físico (mantener pulsado botón BOOT por ~3 segundos)
  if (digitalRead(boot_button) == LOW) {
    delay(100);
    int tiempo_apretado = 0;
    while (digitalRead(boot_button) == LOW) {
      delay(100);
      tiempo_apretado++;
      if (tiempo_apretado > 30) {
        Serial.println("¡Borrando WiFi y reseteando placa a fábrica!");
        RMakerFactoryReset(2); 
      }
    }
  }
  
  // Lógica de Telegram
  if (millis() > lastTimeBotRan + 1000) {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    handleNewMessages(numNewMessages);
    lastTimeBotRan = millis();
  }
  if (millis() - ultimo_reporte_temp > 5000) {
    ultimo_reporte_temp = millis();

    // 1. Acá harías tu analogRead(PIN_SENSOR_TEMP)
    // 2. Aplicás la matemática de la recta para sacar la temperatura a partir de los mV
    
    // Dato inventado para testear:
    float temperatura_calculada = 24.5; 

    // 3. Reportar a RainMaker:
    // Al ser un dispositivo estándar TemperatureSensor, su parámetro interno se llama ESP_RMAKER_DEF_TEMPERATURE_NAME
    sensorTemp.updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, temperatura_calculada);
    
    Serial.printf("Temperatura reportada a la nube: %.2f °C\n", temperatura_calculada);
  }

}