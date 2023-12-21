#ifndef GAMEUI_H
#define GAMEUI_H
#include "../utils/utils.h"

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

typedef struct {
    char pipeName[50];
    char namePlayerSentMessage[30];
    char msg[100];
}MESSAGE;

typedef struct {
    PLAYER *player;
    int stop;
    WINDOW *window;
    int fd;
} PLAYDATA;

typedef struct {
    GAME *game;
    int stop;
    WINDOW *window;
    WINDOW *wInfo;
    int fdRdEngine;
} RECGAMEDATA;

int keyboardCmdGameUI(PLAYER *player, WINDOW *window);

void *threadPlay(void *data);

void *threadRecGame(void *data);

void *threadRecMessages(void *data);

void printmap(GAME game, WINDOW* wGame, WINDOW * wInfo);

void resizeHandler(int sig);

void readMap(int fdRdEngine,WINDOW * window, WINDOW* wInfo);

void handlerSignalGameUI(int signum/*, siginfo_t *info, void *secret*/);

#endif