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
    char name[30];
    int move;   //0 - left, 1 - up, 2 - right, 3 - down, -1 - command, -2 - message
    int position[2];
    char skin;
    char command[30];
    char personNameMessage[30];
    char message[100];
    int accepted;
    int pid;
    char pipeName[30];
} PLAYER;

typedef struct {
    char playerNameSentmsg[20];   //player name that sent the msg
    char pipeName[50];
    char msg[100];
}MESSAGE;

typedef struct {
    PLAYER *player;
    int *stop;
    WINDOW *window;
    pthread_mutex_t *mutexWindow;
} PLAYDATA;

typedef struct {
    GAME *game;
    int *stop;
    WINDOW *window;
    pthread_mutex_t *mutexWindow;
} RECGAMEDATA;

typedef struct {
    GAME *game;
    int *stop;
    WINDOW *window;
    pthread_mutex_t *mutexWindow;
} RECMSGDATA;

int keyboardCmdGameUI(PLAYER *player, WINDOW *window);

void *threadPlay(void *data);

void *threadRecGame(void *data);

void *threadRecMessages(void *data);

#endif