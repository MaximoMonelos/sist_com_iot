#pragma once

#include <Arduino.h>

// Abstract interface for all controllable devices
// This enables scalability: add new devices (Lamp, TV, etc.) by implementing this interface
class IDevice {
public:
    // Initialize the device (called once in setup)
    virtual void begin() = 0;

    // Device loop for periodic tasks (called in main loop)
    virtual void loop() = 0;

    // Primary action: turn on the device (e.g., send WoL packet)
    virtual void onTurnOn() = 0;

    // Get device name for RainMaker and Telegram commands
    virtual const char* getName() = 0;

    // Virtual destructor for proper cleanup
    virtual ~IDevice() = default;
};