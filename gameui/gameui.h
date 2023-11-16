#ifndef GAMEUI_H
#define GAMEUI_H
#include "../utils/utils.h"

typedef struct {
    char name[30];
    int move;   //0 - left, 1 - up, 2 - right, 3 - down
    int position[2];
    int lastposition[2];
    char skin;
    char command[30];
    char personMessage[30];
    char message[100];

} PLAYER;

void keyboardCmdGameUI(PLAYER player, WINDOW *window);

#endif
