#ifndef ENGINE_H
#define ENGINE_H

#include "../utils/utils.h"
#include "../gameui/gameui.h"
#include <ncurses.h>
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
#include <errno.h>

#define BUFFER_SIZE 256

#define TAM 20

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} KBDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} TBDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
    int timeEnrolment;
} ACPDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} CLKDATA;

typedef struct {
    GAME *game;
    pthread_mutex_t *mutexGame;
    int stop;
} PLAYERSDATA;



void createPipe(int *pipeBot, GAME *game);

void launchBot(GAME *game);

void closeBot(GAME *game);

void *threadACP(void *data);

void *threadKBEngine(void *data);

void *threadPlayers(void *data);

void *threadClock(void *data);

void *threadReadBot(void *data);

void readFileMap(int level, GAME *game);

void setEnvVars();

void getEnvVars(GAME *game, ACPDATA *acpData);

void movePlayer(GAME *game, PLAYER *player, DINAMICOBS *obstacle);

void placePlayers(GAME *game);

void passLevel(GAME *game);

void initGame(GAME *game);

void placeRock(int col, int lin, int duration,GAME *game);

void sendMap(GAME *game);

void removeDynamicObstacle(GAME * game);

void insertDynamicObstacle(GAME *game);

void kickPlayer(GAME *game, PLAYER player, int accepted);

void handlerSignalEngine(int signum, siginfo_t *info, void *secret);

/*
ENROLLMENT - time to enroll
NPLAYERS - min of players
DURATION - 1 level time
DECREMENT - decrement of time when level increases
*/

#endif