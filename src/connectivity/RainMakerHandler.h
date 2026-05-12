#pragma once

#include <Arduino.h>
#include <RMaker.h>
#include <WiFiProv.h>
#include <vector>
#include "../devices/IDevice.h"
#include "../config/ConfigManager.h"

// RainMaker handler with device management and config parameters
class RainMakerHandler {
public:
    // Constructor: receives AppConfig and vector of IDevice pointers
    RainMakerHandler(const AppConfig& config, std::vector<IDevice*>& devices);

    // Initialize RainMaker node, device, and parameters
    void begin();

    // Start RainMaker service and BLE provisioning
    void start();

    // Get the Power state (for external access like button handler)
    bool getPCPowerState() const;

    // Set PC Power state externally (for factory reset button)
    void setPCPowerState(bool state);

    // Get pointer to PCDevice for callbacks
    void* getPCDevice() { return _pcDevice; }

    // System event handler (for provisioning events)
    static void sysProvEvent(arduino_event_t *sys_event);

    // Get the power state pointer for RainMaker Switch
    bool* getPowerStatePtr() { return &_pcPowerState; }

private:
    // Write callback for RainMaker parameter changes
    static void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx);

    // Find device by name
    IDevice* findDeviceByName(const String& name);

    const AppConfig& _config;
    std::vector<IDevice*>& _devices;
    
    // RainMaker objects - by value (not pointers)
    Node _myNode;
    Switch _switchDevice;
    Param _tokenParam;
    Param _chatIdParam;
    Param _macParam;
    Param _hostnameParam;

    // Power state (for RainMaker Switch)
    bool _pcPowerState;

    // Pointer to PC device for callbacks
    void* _pcDevice;

    // Static instance for callback access
    static RainMakerHandler* _instance;
};

// Helper: get device by name from vector
IDevice* findDeviceInVector(std::vector<IDevice*>& devices, const char* name);