#ifndef ENGINE_H
#define ENGINE_H

#include "../utils/utils.h"
#include "../gameui/gameui.h"

#define BUFFER_SIZE 256

typedef struct {
    int pid;
    int interval;
    int duration;

} BOT;

typedef struct {
    int position[2];
    int duration;   //tempo que a pedra permanece no mapa
} ROCK;

typedef struct {
    char map[16][40];
    int level;      //3 levels max
    PLAYER players[5];
    int spotsleft;  //spots left to enter the game
    int nPlayers;
    char command[30];
    BOT bots[10];
    int nBots;
    float timeleft;     //inicia com VAR de ambiente DURACAO e a cada nivel passa ser DURACAO-DECREMENTO (outra VAR ambiente)
    ROCK rocks[50];     //max 50
    int timeEnrolment;  //time to enroll
    int minNplayers;    //dado pela VAR de ambiente NPLAYERS
    int *pipeBot;

} GAME;

void createPipe(int *pipeBot);

int launchBot(int *pipeBot, GAME game);

void closeBot(int pid, GAME game);

void keyboardCmdEngine(GAME game);

void readBot(int *pipeBot, int pid);


#endif