#ifndef UTILS_H
#define UTILS_H

#include <ncurses.h>
#include <pthread.h>

#define NCOL 40
#define NLIN 16
#define FIFO_ENGINE_ACP "FIFO_ENGINE_ACP"       // ALL Players -> ENGINE
#define FIFO_GAMEUI "FIFO_GAMEUI_%d"            // ENGINE -> Single Player
#define FIFO_PRIVATE_MSG "FIFO_PRIVATE_MSG_%d"  // Single Player -> Single Player
#define FIFO_PID_MSG "FIFO_PID_MSG_%d"          // Engine -> Single Player
#define FIFO_ENGINE_GAME "FIFO_ENGINE_GAME"     // ALL Players -> ENGINE

typedef struct {
    int pid;
    int interval;
    int duration;
} BOT;

typedef struct {
    int position[2];
    int duration;   //time that the rock stays in the maze
    char skin;
} ROCK;

typedef struct {
    int position[2];
    char skin;
} DINAMICOBS;

typedef struct {
    char name[30];
} NAMEPLAYERS;

typedef struct {
    char name[30];
    int move;   //0 - left, 1 - up, 2 - right, 3 - down, -1 - command, -2 - message
    int position[2];
    char skin;
    char command[30];
    char personNameMessage[30];
    char message[100];
    int accepted;
    int pid;
    NAMEPLAYERS players[5];
} PLAYER;

typedef struct {
    char map[16][40];
    int level;      //3 levels max
    PLAYER players[5];
    int nPlayers;
    PLAYER nonPlayers[10];
    int nNonPlayers;
    BOT bots[10];
    int nBots;
    int time;     //inicia com VAR de ambiente DURACAO e a cada nivel passa ser DURACAO-DECREMENTO (outra VAR ambiente)
    ROCK rocks[50];     //max 50
    DINAMICOBS obstacle[20];
    int nObs;
    int minNplayers;    //dado pela VAR de ambiente NPLAYERS
    int timeDec;        //time decrement VAR de ambiente
    int start;
    int timeleft;       //time to enter the game and time of the
    int *pipeBot;
} GAME;

typedef struct {
    GAME game;
    int *stop;
    WINDOW *window;
} RECMSGDATA;

#endif