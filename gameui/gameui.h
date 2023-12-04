#ifndef GAMEUI_H
#define GAMEUI_H
#include "../utils/utils.h"
#include <ncurses.h>

typedef struct {
    char name[30];
    int move;   //0 - left, 1 - up, 2 - right, 3 - down
    int position[2];
    //int lastposition[2];
    char skin;
    char command[30];
    char personMessage[30];
    char message[100];
    bool accepted;
} PLAYER;

void keyboardCmdGameUI(PLAYER player, WINDOW *window);
void desenhaMoldura(int alt, int comp, int lin, int col);

#endif
