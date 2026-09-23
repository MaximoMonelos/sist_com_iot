#include "devices.h"
#include <string.h>

DeviceType getDeviceType(const char* device_name) {
  
  if (strcmp(device_name, "Configuracion") == 0) return DEVICE_CONFIG;
  if (strcmp(device_name, "Ventilador") == 0)    return DEVICE_FAN;
  if (strcmp(device_name, "Luz") == 0)           return DEVICE_LIGHT;
  if (strcmp(device_name, "Motor Principal") == 0) return DEVICE_MOTOR;
  if (strcmp(device_name, "Sensor Temp") == 0)   return DEVICE_TEMP;
  
  return DEVICE_UNKNOWN;
}

ParamType getParamType(const char* param_name) {
  
  if (strcmp(param_name, "Power") == 0)        return PARAM_POWER;
  if (strcmp(param_name, "Bot Token") == 0)    return PARAM_BOT_TOKEN;
  if (strcmp(param_name, "Chat ID") == 0)      return PARAM_CHAT_ID;
  if (strcmp(param_name, "Umbral_Temp") == 0)  return PARAM_UMBRAL_TEMP;
  if (strcmp(param_name, "RPM_ACTUALES") == 0) return PARAM_RPM_ACT;
  if (strcmp(param_name, "RPM_SETPOINT") == 0) return PARAM_RPM_SET;
  
  return PARAM_UNKNOWN;
}