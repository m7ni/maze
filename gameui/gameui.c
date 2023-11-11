#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gameui.h"


#define TAM 20

int main(int argc, char *argv[]) {
    PLAYER player;
    

    if(argc != 2) {
        printf("\n[ERROR] Invalid number of arguments -> Sintax: <./gameui NAME>\n");
        exit(1);
    }
    
    strcpy(player.name, argv[1]);

    //send player to engine to see if he can enter the game

    while(1) {

        //ver com as ncurses se recebe uma tecla de direcao ou o espaco

        /*
        if(strcmp(,' ')) {
            printf("\nCommand mode active\n");
            keyboardCmd(player);
        }*/
    }
    
    
}

void keyboardCmdGameUI(PLAYER player) {
    char str1[10], str2[30], str3[100];
    char cmd[200];

    while(1) {
        printf("\nCommand: ");
        fflush(stdout);
        fgets(cmd, sizeof(cmd), stdin);
        printf("\n");
        
        if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {
            strcpy(player.command, str1);
            strcpy(player.personMessage, str2);
            strcpy(player.message, str3);
            //send player to engine
            printf("\nVALID");
        } else {
            cmd[ strlen( cmd ) - 1 ] = '\0';
            if(strcmp(cmd, "players") == 0) {
                strcpy(player.command, cmd);
                //send player to engine
                printf("\nVALID");
            }else if(strcmp(cmd, "exit") == 0) {
                strcpy(player.command, cmd);
                printf("\nVALID");
                //avisar o motor com um sinal;
                return;
            } /*else if(strcmp(cmd, ' ') == 0) {
                printf("\nCommand mode over\n");
                return;
            } */
            else {
                printf("\nINVALID");
            }
        }
    }

}
