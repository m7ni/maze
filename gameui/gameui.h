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



typedef struct {
    char name[30];
    int move;   //0 - left, 1 - up, 2 - right, 3 - down
    int position[2];
    char skin;
    char command[30];
    char personMessage[30];
    char message[100];
    int accepted;
    int pid;
    char pname[30];
} PLAYER;

void keyboardCmdGameUI(PLAYER player, WINDOW *window);
void desenhaMoldura(int alt, int comp, int lin, int col);

#endif
