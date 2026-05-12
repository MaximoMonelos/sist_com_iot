#include "RainMakerHandler.h"
#include "../utils/Logger.h"
#include "../devices/PCDevice.h"

// Static instance for callback
RainMakerHandler* RainMakerHandler::_instance = nullptr;

RainMakerHandler::RainMakerHandler(const AppConfig& config, std::vector<IDevice*>& devices)
    : _config(config)
    , _devices(devices)
    , _myNode(RMaker.initNode("ESP32-Smart-PC"))
    , _switchDevice("Computadora", &_pcPowerState)
    , _tokenParam("Bot Token", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE)
    , _chatIdParam("Chat ID", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE)
    , _macParam("MAC PC", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE)
    , _hostnameParam("PC Hostname", "esp.param.string", esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE)
    , _pcPowerState(false)
    , _pcDevice(nullptr)
{
    _instance = this;
}

void RainMakerHandler::begin() {
    LOG_I("RainMaker", "Initializing RainMaker node");

    // Find PC device for state reference
    for (auto device : _devices) {
        if (strcmp(device->getName(), "Computadora") == 0) {
            _pcDevice = device;
            break;
        }
    }

    // Add write callback to the switch
    _switchDevice.addCb(write_callback);

    // Load current config values into parameters
    _tokenParam.updateAndReport(esp_rmaker_str(_config.bot_token));
    _chatIdParam.updateAndReport(esp_rmaker_str(_config.chat_id));
    _macParam.updateAndReport(esp_rmaker_str(_config.pc_mac));
    _hostnameParam.updateAndReport(esp_rmaker_str(_config.pc_hostname));

    // Add parameters to switch device (by value, not pointer)
    _switchDevice.addParam(_tokenParam);
    _switchDevice.addParam(_chatIdParam);
    _switchDevice.addParam(_macParam);
    _switchDevice.addParam(_hostnameParam);

    // Add device to node
    _myNode.addDevice(_switchDevice);

    LOG_I("RainMaker", "Node configured with device and parameters");
}

void RainMakerHandler::start() {
    LOG_I("RainMaker", "Starting RainMaker service");

    // Register system event handler
    WiFi.onEvent(sysProvEvent);

    // Start RainMaker
    RMaker.start();

    // Enable BLE provisioning
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, "pop1234567", "PC_WAKE_BOT");

    LOG_I("RainMaker", "BLE provisioning enabled");
}

void RainMakerHandler::sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            LOG_I("RainMaker", "Provisioning (BLE) Started. Open RainMaker app!");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            LOG_I("RainMaker", "WiFi credentials received. Connecting...");
            break;
        case ARDUINO_EVENT_PROV_END:
            LOG_I("RainMaker", "Provisioning completed successfully");
            break;
        default:
            break;
    }
}

void RainMakerHandler::write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
    if (_instance == nullptr) {
        return;
    }

    const char* deviceName = device->getDeviceName();
    const char* paramName = param->getParamName();

    LOG_I("RainMaker", String("Write callback: ") + deviceName + " / " + paramName);

    // Handle "Computadora" device
    if (strcmp(deviceName, "Computadora") == 0) {
        
        // Power button pressed
        if (strcmp(paramName, "Power") == 0) {
            if (val.val.b == true) {
                LOG_I("RainMaker", "Power ON triggered");
                
                // Find and trigger the device
                for (auto dev : _instance->_devices) {
                    if (strcmp(dev->getName(), "Computadora") == 0) {
                        dev->onTurnOn();
                        break;
                    }
                }

                // Reset button to OFF (simulates a push button)
                param->updateAndReport(esp_rmaker_bool(false));
            }
        }
        // Config parameter: Bot Token
        else if (strcmp(paramName, "Bot Token") == 0) {
            LOG_I("RainMaker", "Bot Token parameter updated");
            param->updateAndReport(val);
        }
        // Config parameter: Chat ID
        else if (strcmp(paramName, "Chat ID") == 0) {
            LOG_I("RainMaker", "Chat ID parameter updated");
            param->updateAndReport(val);
        }
        // Config parameter: MAC PC
        else if (strcmp(paramName, "MAC PC") == 0) {
            LOG_I("RainMaker", "MAC PC parameter updated");
            param->updateAndReport(val);
        }
        // Config parameter: PC Hostname
        else if (strcmp(paramName, "PC Hostname") == 0) {
            LOG_I("RainMaker", "PC Hostname parameter updated");
            param->updateAndReport(val);
        }
    }
}

bool RainMakerHandler::getPCPowerState() const {
    return _pcPowerState;
}

void RainMakerHandler::setPCPowerState(bool state) {
    _pcPowerState = state;
}

IDevice* RainMakerHandler::findDeviceByName(const String& name) {
    for (auto device : _devices) {
        if (strcmp(device->getName(), name.c_str()) == 0) {
            return device;
        }
    }
    return nullptr;
}

IDevice* findDeviceInVector(std::vector<IDevice*>& devices, const char* name) {
    for (auto device : devices) {
        if (strcmp(device->getName(), name) == 0) {
            return device;
        }
    }
    return nullptr;
}