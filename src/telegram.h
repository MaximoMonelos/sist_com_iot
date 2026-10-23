#ifndef TELEGRAM_H
#define TELEGRAM_H

struct system_state_t {
    float temperatura;
    float umbral;
    bool estado_ventilador;
    bool estado_luz; 
};

void handleNewMessages(int numNewMessages, const system_state_t *state);

#endif