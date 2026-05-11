#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <WiFiManager.h>   
#include <ESP32Ping.h>       

char bot_token[50];
char chat_id[15];
char pc_mac[18];
char pc_hostname[30];

bool shouldSaveConfig = false;

// Objetos globales
WiFiClientSecure client;
UniversalTelegramBot *bot;
WiFiUDP udp;
WakeOnLan wol(udp);
WebServer server(80);

bool esperando_pc = false;
unsigned long tiempo_inicio_wol = 0;
unsigned long ultimo_intento_ping = 0;
const unsigned long TIMEOUT_WOL = 180000;   // 3 minutos (en milisegundos) de límite
const unsigned long INTERVALO_PING = 5000;
//const char *pc_ip = "192.168.0.93";
IPAddress ip_descubierta;

unsigned long lastTimeBotRan;
bool pc_status_confirmado = false;

// Callback que se activa cuando hay que guardar configuración
void saveConfigCallback () {
  Serial.println("Se han modificado los parámetros. Guardando...");
  shouldSaveConfig = true;
}

// --- MANEJO DE ARCHIVOS (LittleFS) ---
void saveConfigFile() {
  JsonDocument json;
  json["bot_token"] = bot_token;
  json["chat_id"] = chat_id;
  json["pc_mac"] = pc_mac;
  json["pc_hostname"] = pc_hostname;

  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(json, configFile);
    configFile.close();
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
        strcpy(pc_mac, json["pc_mac"]);
        strcpy(pc_hostname, json["pc_hostname"]);
      }
    }
  }
}

// --- LOGICA DEL SERVIDOR Y TELEGRAM ---
void handlePCReady() {
  server.send(200, "text/plain", "OK");
  pc_status_confirmado = true;
  bot->sendMessage(chat_id, "✅ **PC encendido y reportado via HTTP**", "Markdown");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot->messages[i].text;
    String id = String(bot->messages[i].chat_id);
    if (id != chat_id){
      
      continue;
    };

    if (text == "/start") {
      String welcome = "Bot Configurable:\n/encender\n/reset_wifi (Cuidado)";
      bot->sendMessage(id, welcome, "");
    }
    
   if (text == "/encender") {
      wol.sendMagicPacket(pc_mac);
      bot->sendMessage(id, "🚀 WoL enviado a: " + String(pc_hostname) + "\n⏳ Esperando a que la PC responda en la red...", "");
      
      // Iniciamos la rutina de monitoreo
      esperando_pc = true;
      tiempo_inicio_wol = millis();
      ultimo_intento_ping = millis(); 
    }

    if (text == "/reset_wifi") {
      bot->sendMessage(id, "⚠️ Reiniciando en modo configuración...", "");
      delay(2000);
      WiFiManager wm;
      wm.resetSettings();
      ESP.restart();
    }
  }
}
void find_ip(){
    ip_descubierta = MDNS.queryHost(pc_hostname);
      if (ip_descubierta.toString() != "0.0.0.0") {
        Serial.print("¡IP encontrada automáticamente!: ");
        Serial.println(ip_descubierta);
    } 
    else {
      Serial.println("Buscando la PC en la red...");
    }  
}

void setup() {
  Serial.begin(115200);
  
  // 1. Iniciar Sistema de Archivos
  if (!LittleFS.begin(true)) { Serial.println("Error LittleFS"); return; }
  loadConfigFile();

  // 2. Configurar WiFiManager
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);

  // Campos personalizados en la web
  WiFiManagerParameter custom_bot_token("token", "Telegram Bot Token", bot_token, 50);
  WiFiManagerParameter custom_chat_id("id", "Chat ID", chat_id, 15);
  WiFiManagerParameter custom_pc_mac("mac", "PC MAC (XX:XX:XX...)", pc_mac, 18);
  WiFiManagerParameter custom_pc_hostname("host", "PC Hostname", pc_hostname, 30);

  wm.addParameter(&custom_bot_token);
  wm.addParameter(&custom_chat_id);
  wm.addParameter(&custom_pc_mac);
  wm.addParameter(&custom_pc_hostname);

  // Intenta conectar o abre AP llamado "ESP32_PC_CONTROL"
  if (!wm.autoConnect("ESP32_PC_CONTROL", "password123")) {
    Serial.println("Fallo al conectar. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  // Si llegamos aquí, hay WiFi. Leemos los parámetros del formulario web:
  strcpy(bot_token, custom_bot_token.getValue());
  strcpy(chat_id, custom_chat_id.getValue());
  strcpy(pc_mac, custom_pc_mac.getValue());
  strcpy(pc_hostname, custom_pc_hostname.getValue());

  if (shouldSaveConfig) {
    saveConfigFile();
  }

  // 3. Iniciar servicios con los datos cargados
  client.setInsecure();
  bot = new UniversalTelegramBot(bot_token, client);
  
  if (MDNS.begin("esp32-bot")) Serial.println("mDNS: esp32-bot.local");
  
  server.on("/pc_encendido", handlePCReady);
  server.begin();
  wol.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());

  if(bot_token[0]=='\0' || chat_id[0]=='\0' || pc_mac[0]=='\0'){
    Serial.println("Datos incompletos...");
    Serial.println("Entrando en modo configuracion...");
    wm.resetSettings();
    delay(1000);
    ESP.restart();
  }

  Serial.println("--- Parámetros Cargados ---");
  Serial.print("Token: "); Serial.println(bot_token[0] == '\0' ? "VACÍO" : "Cargado");
  Serial.print("Chat ID: "); Serial.println(chat_id);
  Serial.print("MAC PC: "); Serial.println(pc_mac);
  Serial.println("---------------------------");
}

// void loop() {
//   server.handleClient();
//   if (millis() > lastTimeBotRan + 1000) {
//     int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
//     handleNewMessages(numNewMessages);
//     lastTimeBotRan = millis();
//   }
// }
void loop() {
  server.handleClient();
  
  // Tu lógica de Telegram existente
  if (millis() > lastTimeBotRan + 1000) {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    handleNewMessages(numNewMessages);
    lastTimeBotRan = millis();
  }

  // --- NUEVA LÓGICA DE PING ---
  if (esperando_pc) {
    if (millis() - ultimo_intento_ping >= INTERVALO_PING) {
      ultimo_intento_ping = millis();
      find_ip();
    //   ip_descubierta = MDNS.queryHost(pc_hostname);
    //   if (ip_descubierta.toString() != "0.0.0.0") {
    //     Serial.print("¡IP encontrada automáticamente!: ");
    //     Serial.println(ip_descubierta);
    // } 
    // else {
    //   Serial.println("Buscando la PC en la red...");
    // }  
      Serial.println("Haciendo Ping a la PC...");
      
      // El '1' indica que mande un solo paquete de Ping por ciclo para no trabar el ESP32
      bool ping_exitoso = Ping.ping(ip_descubierta, 1); 

      if (ping_exitoso) {
        Serial.println("Ping exitoso. PC encendida.");
        bot->sendMessage(chat_id, "✅ **PC encendida y conectada a la red (Ping OK)**", "Markdown");
        esperando_pc = false; // Ya prendió, apagamos la bandera
      } 
      else if (millis() - tiempo_inicio_wol >= TIMEOUT_WOL) {
        Serial.println("Timeout de Ping.");
        bot->sendMessage(chat_id, "❌ **Tiempo de espera agotado.** La PC no respondió al Ping después de 3 minutos.", "Markdown");
        esperando_pc = false; // Cancelamos la búsqueda
      }
    }
  }
}