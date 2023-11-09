#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "engine.h"

#define TAM 20

int main(int argc, char *argv[]) {
    GAME game;
    int pipeBot[2];
    createPipe(pipeBot);
    launchBot(pipeBot);
    game.level = 0;
    printf("\nLevel %d\n", game.level);
}

void createPipe(int *pipeBot){
    pipe(pipeBot);
    close(pipeBot[1]);

    //eventualy close pipeBot[0] which is where we read.
}

void launchBot(int *pipeBot){
    int pidBot = fork();
    
    if (pidBot == 0) {
       close(pipeBot[0]);
       dup2(pipeBot[1],1);
       close(pipeBot[1]);

       int res = execl("../bot/bot.exe", "bot", 2,2, NULL);
        if(res!=0){
            perror("Failed Execl\n");
        }
    }
    else if (pidBot > 0) {
        return pidBot;
    }
    else {
        perror("Error creating Bot (Failed Fork())\n");
    }

}

void closeBot(int pid) {
    int status;
    union sigval value;
    value.sival_int = 2;  // You can pass an integer value if needed

    if (sigqueue((pid_t)pid, SIGINT, value) == 0) {
        printf("Sent SIGINT signal to Bot with PID %d\n", pid);
        wait(&status);
            if(WIFEXITED(status)){
                 printf("Bot with PID [%d] terminated (%d)\n",pid,status);
            }
    } else {
        perror("sigqueue");
    }
}