#pragma once

#include <Arduino.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <vector>
#include "../devices/IDevice.h"
#include "../config/ConfigManager.h"

// Telegram Bot handler with device command routing
class TelegramHandler {
public:
    // Constructor: receives AppConfig and vector of IDevice pointers
    TelegramHandler(const AppConfig& config, std::vector<IDevice*>& devices);

    // Initialize bot (call after WiFi is connected)
    void begin();

    // Polling check: returns true if ready to poll
    bool shouldPoll();

    // Handle incoming messages
    void handleNewMessages(int numNewMessages);

    // Main loop call
    void loop();

    // Callback for PC status changes (to send notifications)
    void setOnPCStatusChange(std::function<void(bool)> callback);

private:
    // Build welcome message with registered devices
    String buildWelcomeMessage();

    // Find device by name (case-insensitive)
    IDevice* findDeviceByName(const String& name);

    const AppConfig& _config;
    std::vector<IDevice*>& _devices;
    WiFiClientSecure _client;
    UniversalTelegramBot* _bot;
    unsigned long _lastPollTime;

    // Callback for status changes
    std::function<void(bool)> _onPCStatusChange;

    // Polling interval
    static constexpr unsigned long POLL_INTERVAL_MS = 1000;
};

// Function to convert string to lowercase (for command matching)
String toLowerCase(const String& str);