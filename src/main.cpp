/**
 * ESP32-Smart-PC - Main Entry Point
 * 
 * Modular architecture: delegates all functionality to specialized handlers
 * No global variables - all state encapsulated in classes
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <RMaker.h>

#include "config/ConfigManager.h"
#include "connectivity/RainMakerHandler.h"
#include "connectivity/TelegramHandler.h"
#include "devices/PCDevice.h"
#include "utils/Logger.h"

// Configuration and devices
AppConfig appConfig;
std::vector<IDevice*> devices;

// Handlers (order matters: RainMaker needs to be initialized before WiFi connects)
RainMakerHandler* rainmaker = nullptr;
TelegramHandler* telegram = nullptr;

// Factory reset button pin
static const int FACTORY_RESET_BUTTON = 0;

void setup() {
    Serial.begin(115200);
    LOG_I("Main", "ESP32-Smart-PC starting...");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        LOG_E("Main", "LittleFS mount failed");
        return;
    }
    LOG_I("Main", "LittleFS mounted");

    // Load configuration
    if (!ConfigManager::load(appConfig)) {
        LOG_W("Main", "No config file, using defaults");
    }

    // Initialize button pin
    pinMode(FACTORY_RESET_BUTTON, INPUT_PULLUP);

    // Create device instance
    PCDevice* pcDevice = new PCDevice(appConfig);
    devices.push_back(pcDevice);

    // Create handlers
    rainmaker = new RainMakerHandler(appConfig, devices);
    telegram = new TelegramHandler(appConfig, devices);

    // Initialize RainMaker (node, device, params)
    rainmaker->begin();

    // Start RainMaker and BLE provisioning
    rainmaker->start();

    // Wait for WiFi connection
    LOG_I("Main", "Waiting for WiFi connection...");
    btStop();  // Stop Bluetooth initially
    delay(100);

    // Wait for WiFi to connect
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    LOG_I("Main", String("WiFi connected: ") + WiFi.localIP().toString());

    // Initialize mDNS
    if (!MDNS.begin("esp32-bot")) {
        LOG_E("Main", "mDNS failed to start");
    } else {
        LOG_I("Main", "mDNS started: esp32-bot.local");
    }

    // Initialize Telegram (needs WiFi)
    telegram->begin();

    // Initialize all devices
    for (auto device : devices) {
        device->begin();
    }

    LOG_I("Main", "Setup complete!");
}

void loop() {
    // Factory reset button handling (GPIO0 held > 3 seconds)
    if (digitalRead(FACTORY_RESET_BUTTON) == LOW) {
        delay(100);
        int pressTime = 0;
        while (digitalRead(FACTORY_RESET_BUTTON) == LOW) {
            delay(100);
            pressTime++;
            if (pressTime > 30) {
                LOG_W("Main", "Factory reset triggered!");
                RMakerFactoryReset(2);
                break;
            }
        }
    }

    // Run Telegram handler
    if (telegram != nullptr) {
        telegram->loop();
    }

    // Run all device loops
    for (auto device : devices) {
        device->loop();
    }
}