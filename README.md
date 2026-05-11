# 🚀 ESP32 Telegram PC Wake-on-LAN (WoL) Bot

Este proyecto permite encender una computadora de forma remota a través de Telegram utilizando un **ESP32**. El microcontrolador envía un "Magic Packet" (Wake-on-LAN) a la PC y luego realiza Pings constantes a la red local para avisarte automáticamente cuando la computadora se encendió de forma exitosa.

## ✨ Características Principales

- **Encendido Remoto:** Envío de Magic Packet (WoL) a la placa de red de la PC.
- **Confirmación de Encendido:** Utiliza ICMP (Ping) para detectar cuándo la PC cargó el sistema operativo y está conectada a la red.
- **Portal Cautivo Web:** Configuración de WiFi, Token de Telegram, Chat ID y MAC/Hostname sin tocar el código gracias a `WiFiManager`.
- **Almacenamiento Persistente:** Guarda tu configuración de forma segura en la memoria flash del ESP32 usando `LittleFS` y `ArduinoJson`.
- **Seguridad Básica:** El bot filtra los mensajes y solo responde a tu `Chat ID` específico.

## 🛠️ Requisitos de Hardware

- Placa de desarrollo basada en ESP32.
- Una PC conectada por **cable Ethernet** al mismo router que el ESP32 (el protocolo WoL generalmente no funciona por WiFi).
- La PC debe tener **Wake-on-LAN habilitado** tanto en la BIOS/UEFI como en la configuración de la tarjeta de red en el sistema operativo.

## 📦 Dependencias (PlatformIO)

Asegurate de tener las siguientes librerías en tu archivo `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
  bblanchon/ArduinoJson @ ^7.0.0
  witnessmenow/UniversalTelegramBot @ ^1.3.0
  tzapu/WiFiManager @ ^2.0.17
  a7md0/WakeOnLan @ ^1.1.7
  marian-craciunescu/ESP32Ping @ ^1.7