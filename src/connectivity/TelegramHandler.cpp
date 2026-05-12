#include "TelegramHandler.h"
#include "../utils/Logger.h"

TelegramHandler::TelegramHandler(const AppConfig& config, std::vector<IDevice*>& devices)
    : _config(config)
    , _devices(devices)
    , _bot(nullptr)
    , _lastPollTime(0)
{
}

void TelegramHandler::begin() {
    LOG_I("Telegram", "Initializing Telegram bot");
    _client.setInsecure();
    _bot = new UniversalTelegramBot(_config.bot_token, _client);
    LOG_I("Telegram", "Telegram bot ready");
}

bool TelegramHandler::shouldPoll() {
    return (millis() - _lastPollTime >= POLL_INTERVAL_MS);
}

void TelegramHandler::loop() {
    if (!shouldPoll() || _bot == nullptr) {
        return;
    }

    int numNewMessages = _bot->getUpdates(_bot->last_message_received + 1);
    if (numNewMessages > 0) {
        LOG_I("Telegram", String("New messages: ") + String(numNewMessages));
        handleNewMessages(numNewMessages);
    }

    _lastPollTime = millis();
}

void TelegramHandler::handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String text = _bot->messages[i].text;
        String chatId = String(_bot->messages[i].chat_id);

        // Validate chat_id
        if (chatId != _config.chat_id) {
            LOG_W("Telegram", "Message from unknown chat, ignoring");
            continue;
        }

        // Parse commands
        if (text == "/start") {
            _bot->sendMessage(chatId, buildWelcomeMessage(), "");
        }
        else if (text == "/estado") {
            String statusMsg = "Estado de dispositivos:\n";
            for (auto device : _devices) {
                statusMsg += "- " + String(device->getName()) + "\n";
            }
            _bot->sendMessage(chatId, statusMsg, "");
        }
        else if (text == "/encender") {
            // Find "Computadora" device
            IDevice* pc = findDeviceByName("Computadora");
            if (pc != nullptr) {
                pc->onTurnOn();
                _bot->sendMessage(chatId, 
                    String("🚀 WoL enviado a: ") + String(_config.pc_hostname) + 
                    "\n⏳ Esperando a que la PC responda en la red...", "");
            }
        }
        else {
            // Check for device-specific commands (e.g., /computadora)
            String cmdPrefix = "/";
            if (text.startsWith(cmdPrefix)) {
                String deviceName = text.substring(1);  // Remove "/"
                deviceName.toLowerCase();
                
                IDevice* device = findDeviceByName(deviceName);
                if (device != nullptr) {
                    device->onTurnOn();
                    _bot->sendMessage(chatId, 
                        String("🚀 Encendiendo: ") + device->getName(), "");
                } else {
                    _bot->sendMessage(chatId, 
                        String("Dispositivo '") + deviceName + "' no encontrado", "");
                }
            }
        }
    }
}

String TelegramHandler::buildWelcomeMessage() {
    String msg = "🤖 Bot de Control ESP32\n\n";
    msg += "Comandos disponibles:\n";
    msg += "/start - Mostrar este mensaje\n";
    msg += "/estado - Ver dispositivos\n";
    msg += "/encender - Encender PC\n\n";
    msg += "Dispositivos registrados:\n";
    for (auto device : _devices) {
        msg += "- " + String(device->getName()) + "\n";
    }
    return msg;
}

IDevice* TelegramHandler::findDeviceByName(const String& name) {
    String lowerName = name;
    lowerName.toLowerCase();
    
    for (auto device : _devices) {
        String deviceName = device->getName();
        deviceName.toLowerCase();
        if (deviceName == lowerName) {
            return device;
        }
    }
    return nullptr;
}

void TelegramHandler::setOnPCStatusChange(std::function<void(bool)> callback) {
    _onPCStatusChange = callback;
}

// Helper function implementation
String toLowerCase(const String& str) {
    String result = str;
    result.toLowerCase();
    return result;
}