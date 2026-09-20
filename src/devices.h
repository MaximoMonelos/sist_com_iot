#ifndef DEVICES_H
#define DEVICES_H

// 1. Enum de Dispositivos
enum DeviceType {
  DEVICE_CONFIG,
  DEVICE_FAN,
  DEVICE_LIGHT,
  DEVICE_MOTOR,
  DEVICE_TEMP,
  DEVICE_UNKNOWN
};

// 2. Enum de Parámetros
enum ParamType {
  PARAM_POWER,
  PARAM_BOT_TOKEN,
  PARAM_CHAT_ID,
  PARAM_TEMP,
  PARAM_UMBRAL_TEMP,
  PARAM_RPM_ACT,
  PARAM_RPM_SET,
  PARAM_UNKNOWN
};

// 3. Declaración de las funciones
DeviceType getDeviceType(const char* device_name);
ParamType getParamType(const char* param_name);

#endif