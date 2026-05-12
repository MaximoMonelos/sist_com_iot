#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <ESP32Ping.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RMaker.h>
#include <WiFiProv.h>
#include <ESPmDNS.h>


char bot_token[100] = "";
char chat_id[15] = "";
char pc_mac[18] = "";
char pc_hostname[30] = "";

bool shouldSaveConfig = false;

// Objetos globales
WiFiClientSecure client;
UniversalTelegramBot *bot;
WiFiUDP udp;
WakeOnLan wol(udp);
//WebServer server(80);

bool esperando_pc = false;
unsigned long tiempo_inicio_wol = 0;
unsigned long ultimo_intento_ping = 0;
const unsigned long TIMEOUT_WOL = 180000;   // 3 minutos (en milisegundos) de límite
const unsigned long INTERVALO_PING = 5000;
//const char *pc_ip = "192.168.0.93";
IPAddress ip_descubierta;

unsigned long lastTimeBotRan;
bool pc_status_confirmado = false;
const int boot_button = 0;
static Switch my_switch("Computadora", &pc_status_confirmado);

Param token_param("Bot Token", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
Param chat_id_param("Chat ID", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
Param mac_param("MAC PC", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
Param hostname_param("PC Hostname", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);

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
    Serial.println("✅ Archivo guardado correctamente en LittleFS!");
  }
  else{
    Serial.println("❌ ERROR FATAL: No se pudo crear config.json");
    return;
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

// --------- LOGICA DEL SERVIDOR Y TELEGRAM -----------
// void handlePCReady() {
//   server.send(200, "text/plain", "OK");
//   pc_status_confirmado = true;
//   bot->sendMessage(chat_id, "✅ **PC encendido y reportado via HTTP**", "Markdown");
// }

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

    // if (text == "/reset_wifi") {
    //   bot->sendMessage(id, "⚠️ Reiniciando en modo configuración...", "");
    //   delay(2000);
    //   WiFiManager wm;
    //   wm.resetSettings();
    //   ESP.restart();
    // }
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

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();

  if (strcmp(device_name, "Computadora") == 0) {
    
    // 1. Si tocaron el botón de ENCENDIDO
    if (strcmp(param_name, "Power") == 0) {
      if (val.val.b == true) { // Si el botón se puso en ON
        wol.sendMagicPacket(pc_mac);
        bot->sendMessage(chat_id, "📱 **WoL enviado vía App/Google Home**", "Markdown");
        esperando_pc = true;
        tiempo_inicio_wol = millis();
        ultimo_intento_ping = millis();
        
        // Volvemos el botón a OFF en la app automáticamente (simula un pulsador)
        param->updateAndReport(esp_rmaker_bool(false)); 
      }
    }
    
   else if (strcmp(param_name, "Bot Token") == 0) {
      memset(bot_token, 0, sizeof(bot_token)); // Limpiamos el buffer antes de copiar
      strncpy(bot_token, val.val.s, sizeof(bot_token) - 1);
      param->updateAndReport(val); // <--- ESTO ES VITAL: Confirma el dato en la App
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
      param->updateAndReport(val); // <--- Confirma el dato
      saveConfigFile();
      Serial.println("Chat ID guardado.");
    }
    else if (strcmp(param_name, "MAC PC") == 0) {
      memset(pc_mac, 0, sizeof(pc_mac));
      strncpy(pc_mac, val.val.s, sizeof(pc_mac) - 1);
      param->updateAndReport(val); // <--- Confirma el dato
      saveConfigFile();
    }
    else if (strcmp(param_name, "PC Hostname") == 0) {
      memset(pc_hostname, 0, sizeof(pc_hostname));
      strncpy(pc_hostname, val.val.s, sizeof(pc_hostname) - 1);
      param->updateAndReport(val); // <--- Confirma el dato
      saveConfigFile();
      Serial.println("Hostname guardado.");
    }
  }
}



void setup() {
  Serial.begin(115200);
  pinMode(boot_button, INPUT_PULLUP); // Botón físico de reset
  
  if (!LittleFS.begin(true)) { Serial.println("Error LittleFS"); return; }
  loadConfigFile(); // Cargamos tus datos viejos

  // --- CONFIGURACIÓN DEL NODO RAINMAKER ---
  Node my_node = RMaker.initNode("ESP32-Smart-PC");

  // Enganchamos la función callback que definimos antes
  my_switch.addCb(write_callback);
  
  
  // Le cargamos los valores actuales a los campos de texto
  token_param.updateAndReport(esp_rmaker_str(bot_token));
  chat_id_param.updateAndReport(esp_rmaker_str(chat_id));
  mac_param.updateAndReport(esp_rmaker_str(pc_mac));
  hostname_param.updateAndReport(esp_rmaker_str(pc_hostname));
  
  // Metemos los campos de texto ADENTRO del switch (para que aparezcan juntos en la app)
  
  my_switch.addParam(token_param);
  my_switch.addParam(chat_id_param);
  my_switch.addParam(mac_param);
  my_switch.addParam(hostname_param);
  
  // Agregamos el switch al nodo principal
  my_node.addDevice(my_switch);
  

  // --- INICIO DE SERVICIOS ---
  // Empieza a escuchar el WiFi
  RMaker.start(); // Prepara RainMaker
  WiFi.onEvent(sysProvEvent);

  // --- EL TRUCO MAGICO PARA GANAR LA APUESTA ---
  WiFi.mode(WIFI_STA); // Obligamos al WiFi a estar en modo estación
  btStop();            // Apagamos el Bluetooth rebelde que inició Arduino
  delay(100);           // Le damos un respiro al hardware
  
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, "pop1234567", "PC_WAKE_BOT");

  // Esto prende el Bluetooth. "pop1234567" es el PIN que te va a pedir la app
  Serial.print("Esperando conexión WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi Conectado!");
  
  if (!MDNS.begin("esp32-bot")) {
    Serial.println("Error iniciando mDNS");
  }
  // Preparamos Telegram y WoL
  client.setInsecure();
  bot = new UniversalTelegramBot(bot_token, client);
  wol.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Necesario para mandar el WoL
}

void loop() {
  if (digitalRead(boot_button) == LOW) {
    delay(100);
    int tiempo_apretado = 0;
    while (digitalRead(boot_button) == LOW) {
      delay(100);
      tiempo_apretado++;
      if (tiempo_apretado > 30) {
        Serial.println("¡Borrando WiFi y reseteando placa a fábrica!");
        RMakerFactoryReset(2); // Formatea RainMaker
      }
    }
  }
  
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