#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gameui.h"


#define TAM 20

int main(int argc, char *argv[]) {
    PLAYER player;
    int flag = 0, i = 0;
    char str1[10], str2[30], str3[100];
    char cmd[200];

    if(sizeof(argv) != 2) {
        printf("\n[ERROR] Invalid number of arguments -> Sintax: <./gameui NAME>\n");
        exit(1);
    }
    
    strcpy(player.name, argv[1]);
/*
    for(i = 0 ; i < 5 ; i++) {
        if(game.players[i].name != NULL) {
            if(strcmp(game.players[i].name, player.name) == 0) {
                flag = 1;
                printf("\nThere are already someone with that name\n");
                exit(1);
            }
        } else 
            break;
    }

    if(flag == 0)
        game.players[i] = player;
    */
    while(1) {
        printf("\nCommand: ");
        fflush(stdout);
        scanf("%s", &cmd);
        printf("\n");
        
        if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {
            strcpy(player.command, str1);
            strcpy(player.personMessage, str2);
            strcpy(player.message, str3);
        } else {
            if(strcmp(cmd, "players") == 0) {
                strcpy(player.command, cmd);
                printf("\nVALID");
            }else if(strcmp(cmd, "exit") == 0) {
                strcpy(player.command, cmd);
                printf("\nVALID");
                //avisar o motor com um sinal;
            } else {
                printf("\nINVALID");
            }
        }
    }
}

