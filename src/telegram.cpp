#include "telegram.h"
#include <Arduino.h>
#include <UniversalTelegramBot.h>

// Le avisamos que estas variables existen en el main.cpp
extern UniversalTelegramBot *bot;
extern char chat_id[15];

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot->messages[i].text;
    String id = String(bot->messages[i].chat_id);
    
    if (id != chat_id){
      continue; // Ignora mensajes de gente que no seas vos
    }
    
    if (text == "/start") {
      String welcome = "Bot de Comunicaciones:\n/estado - Ver estado del sistema\n/reset_wifi (Cuidado)";
      bot->sendMessage(id, welcome, "");
    }
    
    if (text == "/estado") {
      bot->sendMessage(id, "El sistema está funcionando. (Acá irán los datos)", "");
    }
  }
}