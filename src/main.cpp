#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RMaker.h>
#include <WiFiProv.h>
#include "config_fs.h"
#include "telegram.h"
#include "devices.h"

#define PIN_RELE 26
#define PIN_FAN_CONTROL 25
#define PIN_SENSOR_TEMP 34


char bot_token[100] = "";
char chat_id[15] = "";

// Objetos globales
WiFiClientSecure client;
UniversalTelegramBot *bot;

unsigned long lastTimeBotRan;
unsigned long ultimo_reporte_temp = 0;
const int boot_button = 0; // Botón físico de BOOT del ESP32
volatile float umbral = 50; // SE PRENDE A LOS 50°C POR DEFAULT, SE PUEDE CAMBIAR EL LA APP
bool FAN = false;

unsigned long ultimo_muestreo_temp = 0;
uint32_t acumulador_adc = 0; 
int contador_muestras = 0;


Device config_device("Configuracion"); //dispositivo para los parámetros de configuración del bot

Param token_param("Bot Token", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
Param chat_id_param("Chat ID", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);

//Dispositivos estándar RainMaker
TemperatureSensor sensorTemp("Sensor Temp"); 

Switch actuadorFan("Ventilador");
Param umbral_temp("Umbral_Temp", "esp.param.Umbral_Temp",value(50.0f), PROP_FLAG_READ | PROP_FLAG_WRITE);

Switch releLuz("Luz");


void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START: Serial.println("Provisioning (BLE) Iniciado. Abrí la app RainMaker!"); break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS: Serial.println("WiFi Correcto! Conectando..."); break;
    case ARDUINO_EVENT_PROV_END: Serial.println("Conexión finalizada exitosamente."); break;
    default: break;
  }
}


void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();
  
  DeviceType current_device = getDeviceType(device_name);
  ParamType current_param = getParamType(param_name);

  switch (current_device) {
    
    case DEVICE_CONFIG:
      
      switch (current_param) {
        
        case PARAM_BOT_TOKEN:
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
          break;

        case PARAM_CHAT_ID:
          memset(chat_id, 0, sizeof(chat_id));
          strncpy(chat_id, val.val.s, sizeof(chat_id) - 1);
          param->updateAndReport(val);
          saveConfigFile();
          Serial.println("Chat ID guardado.");
          break;
        default: break;
      }
      break;

    case DEVICE_FAN:
      
      if (current_param == PARAM_POWER) {
        bool estado = val.val.b;
        digitalWrite(PIN_FAN_CONTROL, estado);
        param->updateAndReport(val);
      }
      else if (current_param == PARAM_UMBRAL_TEMP){
        umbral = val.val.f;
        Serial.printf("El umbral de temperatura fue seteado a %.2f°C\n", umbral);
        param->updateAndReport(val);
        saveConfigFile();
      }
      break;

    case DEVICE_LIGHT:
      
      if(current_param == PARAM_POWER){
        bool estado_luz = val.val.b;
        digitalWrite(PIN_RELE, !estado_luz);
        param->updateAndReport(val);
      }
      break;

    case DEVICE_TEMP:
      
      if (current_param == PARAM_UMBRAL_TEMP) {
        // float umbral = val.val.f;
        // param->updateAndReport(val);
      }
      break;

    case DEVICE_UNKNOWN:
    default:
      break;
  }
}


void setup() {
 
  Serial.begin(115200);
  
  pinMode(boot_button, INPUT_PULLUP); // Botón físico de reset
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_FAN_CONTROL, OUTPUT);
  digitalWrite(PIN_RELE, HIGH);  //Luz Apagada por default
  digitalWrite(PIN_FAN_CONTROL, LOW); //Fan Apagado por default

  analogReadResolution(12);
  


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
  
  

  // C. Enganchar los callbacks de los switches estándar
  actuadorFan.addCb(write_callback);
  actuadorFan.addParam(umbral_temp);
  releLuz.addCb(write_callback);
  umbral_temp.updateAndReport(esp_rmaker_float(umbral));

  // D. Colgar todos los Dispositivos al Nodo Principal
  my_node.addDevice(config_device);
  my_node.addDevice(sensorTemp);
  my_node.addDevice(actuadorFan);
  my_node.addDevice(releLuz);


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
  // Entra acá cada 100 ms
  if (millis() - ultimo_muestreo_temp >= 50) {
    ultimo_muestreo_temp = millis();

    // 1. Tomamos la muestra y la sumamos al pozo
    acumulador_adc += analogRead(PIN_SENSOR_TEMP);
    contador_muestras++;

    // 2. Si ya juntamos 100 muestras (100 * 50ms = 5000ms = 5 segundos)
    if (contador_muestras >= 100) {
      
      
      float adc_promedio = (float)acumulador_adc / 50.0;

      
      float voltaje_mv = (adc_promedio / 4095.0) * 3300.0;

      
      float ganancia_av = 22.0;
      float temperatura_calculada = ((voltaje_mv / ganancia_av) + 50.0) / 2.0;

      Serial.printf("ADC Promedio: %.1f | Temp: %.2f °C\n", adc_promedio, temperatura_calculada);

      
      sensorTemp.updateAndReportParam(ESP_RMAKER_DEF_TEMPERATURE_NAME, temperatura_calculada);

    
      acumulador_adc = 0;
      contador_muestras = 0;
     
      if (temperatura_calculada >= umbral && !FAN) {
          
          FAN = true; 
          
          digitalWrite(PIN_FAN_CONTROL, FAN);
          actuadorFan.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, FAN);
          
          String mensaje_alerta = "⚠️ Alerta térmica! Se encendió automáticamente el ventilador, Temperatura: " + String(temperatura_calculada, 1) + " °C";
          esp_rmaker_raise_alert(mensaje_alerta.c_str());

          if (strlen(chat_id) > 0) {
          
            bot->sendMessage(chat_id, mensaje_alerta, "");
        
          } 
                    
      } 
      // Si bajó la temperatura Y el ventilador sigue prendido
      else if (temperatura_calculada < umbral && FAN) {
          
          FAN = false; 
          
          digitalWrite(PIN_FAN_CONTROL, FAN);
          actuadorFan.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, FAN);
          
          String mensaje_ok = "✅ Temperatura estabilizada (" + String(temperatura_calculada, 1) + " °C). Ventilador apagado.";
          esp_rmaker_raise_alert(mensaje_ok.c_str());

          if (strlen(chat_id) > 0) {
          
            bot->sendMessage(chat_id, mensaje_ok, "");
        
          } 
        }
      } 
    } 
}