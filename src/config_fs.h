#ifndef CONFIG_FS_H
#define CONFIG_FS_H

#include <Arduino.h>

// Declaramos que estas funciones existen y se pueden usar desde cualquier lado
void saveConfigFile();
void loadConfigFile();

#endif
