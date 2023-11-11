#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "engine.h"
#include <unistd.h>
#include <fcntl.h>

#define TAM 20

int main(int argc, char *argv[]) {
    GAME game;
    int pipeBot[2];
    createPipe(pipeBot);
    game.pipeBot = pipeBot;

    game.bots[0].pid = launchBot(game.pipeBot,game);
    readBot(game.pipeBot);
    closeBot(game.bots[0].pid, game);
    //keyboardCmdEngine(game);


}

void createPipe(int *pipeBot){
    pipe(pipeBot);
    close(pipeBot[1]);

    //eventualy close pipeBot[0] which is where we read.
}

void readBot(int *pipeBot) {
    char buffer[BUFFER_SIZE];
    int bytes_read,i=0;
    printf("\npipebot %d\n", pipeBot[0]);
    while(i<3){
        // Read from the pipe
        bytes_read = read(pipeBot[0], buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            perror("read");
        } else {
            buffer[bytes_read] = '\0';  // Null-terminate the received data

            // Print the received string
            printf("Received: %s\n", buffer);
        }
        i++;
    }
}

int launchBot(int *pipeBot, GAME game){
    int pidBot = fork();

    if (pidBot == 0) {
        close(pipeBot[0]);
        if (dup2(pipeBot[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipeBot[1]);

       int res = execl("bot/bot.exe", "bot", "2","2", NULL);
        if(res!=0){
            perror("Failed Execl ");
        }
    }
    else if (pidBot > 0) {
        game.nBots++;
        return pidBot;
    }
    else {
        perror("Error creating Bot (Failed Fork())");
    }

}

void closeBot(int pid, GAME game) {
    int status;
    union sigval value;
    value.sival_int = 2;  // You can pass an integer value if needed

    if (sigqueue((pid_t)pid, SIGINT, value) == 0) {
        printf("Sent SIGINT signal to Bot with PID %d\n", pid);
        wait(&status);
            if(WIFEXITED(status)){
                printf("Bot with PID [%d] terminated (%d)\n",pid,status);
                game.nBots--;
            }
    } else {
        perror("sigqueue");
    }
} 

void acceptPlayer(PLAYER player, GAME game) {
    int flag = 0, i = 0;

    for(i = 0 ; i < game.nPlayers ; i++) {
        if(strcmp(game.players[i].name, player.name) == 0) {
            flag = 1;
            printf("\nThere are already a player with that name\n");
            exit(1);
        } else 
            break;
    }

    if(flag == 0) {
        game.players[i] = player;
        printf("\nPlayer %s [%d] was entered the game\n", player.name, i);
    }
    game.nPlayers++;
}

void keyboardCmdEngine(GAME game) {
    char cmd[200], str1[30], str2[30];
    
    while(1) {
        int flag = 0;
        printf("\nCommand: ");
        fflush(stdout);
        fgets(cmd, sizeof(cmd), stdin);
        printf("\n");
        
        
        if (sscanf(cmd, "%s %s", str1, str2) == 2 && strcmp(str1, "kick") == 0) {
            for(int i = 0 ; i < game.nPlayers ; i++) {
                if(strcmp(game.players[i].name, str2)) {
                    printf("\nPlayer %s has been kicked\n", game.players[i].name);
                    //send signal to player and remove him from the game
                    printf("\nVALID");
                    flag = 1;
                    break;
                }
            }
            if(flag == 0) {
                printf("\nThere is no player with the name %s\n", str2);
                printf("\nINVALID");
            }
        } else {
            cmd[ strlen( cmd ) - 1 ] = '\0';
            if(strcmp(cmd, "users") == 0) {
                printf("\nPlayers:\n");
                for(int i = 0 ; i < game.nPlayers ; i++) {
                    printf("\tPlayer %s\n", game.players[i].name);
                }
                printf("\nVALID");
            } else if(strcmp(cmd, "bots") == 0) {
                printf("\nVALID");
                if(game.nBots == 0) {
                    printf("\nThere are no bots yet\n");
                    break;
                }
                printf("\nBots:\n");
                for(int i = 0 ; i < game.nBots ; i++) {
                    printf("Bot %d with PID[%d]\t interval [%d]\t duration [%d]", 
                    i, game.bots[i].pid, game.bots[i].interval, game.bots[i].duration);
                }
            } 
            else if(strcmp(cmd, "bmov") == 0) {
                printf("\nVALID");

            } 
            else if(strcmp(cmd, "rbm") == 0) {
                printf("\nVALID");

            } 
            else if(strcmp(cmd, "begin") == 0) {  //init the game automatically even if there is no min number of Players
                printf("\nVALID");

            } else if(strcmp(cmd, "bot") == 0) {
                printf("\nVALID");
                game.bots[0].pid = launchBot(game.pipeBot,game);
                readBot(game.pipeBot);
                closeBot(game.bots[0].pid, game);
                
            } 
            else if(strcmp(cmd, "exit") == 0) {
                printf("\nVALID\n");
                //avisar os bots com um sinal;
                //avisar os jogadores;
                return;
            } 
            else {
                printf("\nINVALID");
            } 
        }
        
    }
}

void readFileMap(int level, GAME *game) {
    FILE *file;
    char fileName[30];

    switch(level) {
        case 1:
            strcpy(fileName, "level1map.txt");
        break;

        case 2:
            strcpy(fileName, "level2map.txt");
        break;

        case 3:
            strcpy(fileName, "level3map.txt");
        break;
    }

    // Open the file for reading
    file = fopen(fileName, "r");

    // Check if the file was opened successfully
    if (file == NULL) {
        perror("\nError opening file\n");
        return; // Return an error code
    }

    // Read the content of the file into the array
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 40; ++j) {
            int ch = fgetc(file);

            // Check for end of file or error
            if (ch == EOF || ch == '\n') {
                break;
            }

            game->map[i][j] = (char)ch;
        }

        // Skip remaining characters in the line if it's longer than 40 characters
        int c;
        while ((c = fgetc(file)) != EOF && c != '\n')
            ;
    }

    // Close the file
    fclose(file);

    // Print the content of the array with explicit newline characters
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 40; ++j) {
            printf("%c", game->map[i][j]);
        }
        printf("\n");
    }

    return 0; // Return success code
}


