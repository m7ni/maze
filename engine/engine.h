#ifndef ENGINE_H
#define ENGINE_H

#include "../utils/utils.h"
#include "../gameui/gameui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <error.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 256

#define ENGINE_FIFO_ACP "ENGINE_FIFO_ACP"

#define TAM 20
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
    char map[16][40];
    int level;      //3 levels max
    PLAYER players[5];
    int nPlayers;
    PLAYER nonPlayers[10];
    int nNonPlayers;
    BOT bots[10];
    int nBots;
    float timeleft;     //inicia com VAR de ambiente DURACAO e a cada nivel passa ser DURACAO-DECREMENTO (outra VAR ambiente)
    ROCK rocks[50];     //max 50
    DINAMICOBS obstacle[20];
    int minNplayers;    //dado pela VAR de ambiente NPLAYERS
    int timeDec;        //time decrement VAR de ambiente
    int *pipeBot;
} GAME;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} KBDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
    int timeEnrolment; //time to enroll VAR de ambiente
} ACPDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} PLAYERSDATA;

void createPipe(int *pipeBot, GAME *game);

int launchBot(int *pipeBot, GAME *game);

void closeBot(int pid, GAME *game);

void *threadACP(void *data);

void *threadKBEngine(void *data);

void *threadPlayers(void *data);

void readBot(int *pipeBot, int pid);

void readFileMap(int level, GAME *game);

void setEnvVars();

void getEnvVars(GAME *game, ACPDATA *acpData);

void movePlayer(GAME *game, PLAYER *player, DINAMICOBS *obstacle);

void placePlayers(GAME *game);

void passLevel(GAME *game);

void initGame(GAME *game);

/*
ENROLLMENT - time to enroll
NPLAYERS - min of players
DURATION - 1 level time
DECREMENT - decrement of time when level increases
*/

#endif