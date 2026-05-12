#include "PCDevice.h"
#include "../utils/Logger.h"
#include <ESP32Ping.h>
#include <ESPmDNS.h>

PCDevice::PCDevice(const AppConfig& config)
    : _config(config)
    , _wol(_udp)
    , _discoveredIP(0, 0, 0, 0)
    , _waitingForPC(false)
    , _wolStartTime(0)
    , _lastPingTime(0)
    , _pcOn(false)
{
}

void PCDevice::begin() {
    LOG_I("PCDevice", "Initializing PC Device");
    _wol.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
    _discoveredIP = IPAddress(0, 0, 0, 0);
    _waitingForPC = false;
    _pcOn = false;
    LOG_I("PCDevice", String("WoL configured for MAC: ") + _config.pc_mac);
}

void PCDevice::loop() {
    if (_waitingForPC) {
        checkPCStatus();
    }
}

void PCDevice::onTurnOn() {
    LOG_I("PCDevice", "Turn on requested");
    sendWOL();

    // Start monitoring state machine
    _waitingForPC = true;
    _wolStartTime = millis();
    _lastPingTime = millis();
    _pcOn = false;

    LOG_I("PCDevice", "WoL sent, waiting for PC to respond");
}

const char* PCDevice::getName() {
    return DEVICE_NAME;
}

void PCDevice::setOnStatusChange(std::function<void(bool)> callback) {
    _onStatusChange = callback;
}

void PCDevice::sendWOL() {
    _wol.sendMagicPacket(_config.pc_mac);
    LOG_I("PCDevice", "Magic packet sent");
}

void PCDevice::discoverIP() {
    _discoveredIP = MDNS.queryHost(_config.pc_hostname);
    if (_discoveredIP.toString() != "0.0.0.0") {
        LOG_I("PCDevice", String("PC IP discovered: ") + _discoveredIP.toString());
    } else {
        LOG_W("PCDevice", "PC not found via mDNS");
    }
}

void PCDevice::checkPCStatus() {
    // Check timeout
    if (millis() - _wolStartTime >= TIMEOUT_MS) {
        LOG_W("PCDevice", "Timeout: PC did not respond");
        _waitingForPC = false;
        if (_onStatusChange) {
            _onStatusChange(false);  // Notify timeout/failure
        }
        return;
    }

    // Periodic ping check
    if (millis() - _lastPingTime >= PING_INTERVAL_MS) {
        _lastPingTime = millis();

        // First try mDNS discovery
        discoverIP();

        // If we have a valid IP, try ping
        if (_discoveredIP.toString() != "0.0.0.0") {
            LOG_I("PCDevice", "Pinging PC...");
            bool pingSuccess = Ping.ping(_discoveredIP, 1);

            if (pingSuccess) {
                LOG_I("PCDevice", "PC is online!");
                _pcOn = true;
                _waitingForPC = false;
                if (_onStatusChange) {
                    _onStatusChange(true);  // Notify success
                }
            } else {
                LOG_W("PCDevice", "Ping failed, will retry...");
            }
        }
    }
}