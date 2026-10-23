#include "telegram.h"
#include <Arduino.h>
#include <UniversalTelegramBot.h>

// Le avisamos que estas variables existen en el main.cpp
extern UniversalTelegramBot *bot;
extern char chat_id[15];


void handleNewMessages(int numNewMessages, const system_state_t *state) {
  
  for (int i = 0; i < numNewMessages; i++) {
    
    String text = bot->messages[i].text;
    String id = String(bot->messages[i].chat_id);
    
    if (id != chat_id){
      continue; // Ignora mensajes de gente que no seas vos
    }
    
    if (text == "/help") {

      String msj = "👋 ¡Hola! Soy el nodo ESP32 de tu proyecto.\n\n";
      msj += "📋 *Comandos disponibles:*\n";
      msj += "🔹 /estado - Muestra los valores actuales de los sensores\n";
      msj += "🔹 /help - Muestra este menú de ayuda\n\n";
      msj += "⚙️ _Nota: El umbral térmico y la luz se configuran desde RainMaker._";
      
      bot->sendMessage(id, msj, "Markdown");

    }
    
    if (text == "/estado") {
    
      String msj = "📊 *Estado del Sistema*\n\n";
     
      msj += "🌡️ Temp. actual: " + String(state->temperatura, 1) + " °C\n";
      msj += "🎯 Umbral seteado: " + String(state->umbral, 1) + " °C\n";
      msj += "❄️ Ventilador: ";
      msj += state->estado_ventilador ? "Encendido 🟢\n" : "Apagado 🔴\n";
      msj += "💡 Luz: ";
      msj += state->estado_luz ? "Encendida 🟢\n" : "Apagada 🔴\n";
      bot->sendMessage(id, msj, "Markdown");
    }
  }
}