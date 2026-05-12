#pragma once

#include "IDevice.h"
#include "../config/ConfigManager.h"
#include <functional>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <IPAddress.h>

// PC Device: implements Wake-on-LAN + ping + mDNS discovery
class PCDevice : public IDevice {
public:
    // Constructor: receives reference to AppConfig (no globals)
    explicit PCDevice(const AppConfig& config);

    // IDevice interface implementation
    void begin() override;
    void loop() override;
    void onTurnOn() override;
    const char* getName() override;

    // Callback to notify status changes (for Telegram/RainMaker)
    void setOnStatusChange(std::function<void(bool)> callback);

    // Get current PC status (for RainMaker switch)
    bool getStatus() const { return _pcOn; }

private:
    // Internal state machine
    void checkPCStatus();

    // Send Wake-on-LAN magic packet
    void sendWOL();

    // Discover PC IP via mDNS
    void discoverIP();

    const AppConfig& _config;
    WiFiUDP _udp;
    WakeOnLan _wol;
    IPAddress _discoveredIP;

    // State tracking
    bool _waitingForPC;
    unsigned long _wolStartTime;
    unsigned long _lastPingTime;
    bool _pcOn;

    // Timeout constants
    static constexpr unsigned long TIMEOUT_MS = 180000;  // 3 minutes
    static constexpr unsigned long PING_INTERVAL_MS = 5000;

    // Callback for status changes
    std::function<void(bool)> _onStatusChange;

    // Device name constant
    static constexpr const char* DEVICE_NAME = "Computadora";
};