#include "engine.h"

int main(int argc, char *argv[]) {
    ACPDATA acpData;
    KBDATA kbData;
    PLAYERSDATA plData;
    CLKDATA clkData;
    GAME game;
    int pipeBot[2],timeEnrolment;
    pthread_t threadACPID, threadKBID, threadPlayersID, threadClockID;
    pthread_mutex_t mutexGame;
    int stop = 0;

    //Creating FIFO for accepting PLAYERS
    if(mkfifo(FIFO_ENGINE_ACP, 0666) == -1) {
        perror("Error opening fifo or another engine is already running\n"); 
        return -1;
    }   
    setEnvVars();
    getEnvVars(&game, &acpData);
    
    pthread_mutex_init(&mutexGame, NULL);
    
    acpData.game = &game;
    acpData.stop = &stop;
    acpData.mutexGame = &mutexGame;
    
    if(pthread_create(&threadACPID, NULL, &threadACP, &acpData)) {
        perror("Error creating threadACP\n");
        return -1;
    }
    printf("\nCreated thread ACP");
    
    clkData.game = &game;
    clkData.stop = &stop;
    clkData.mutexGame = &mutexGame;
    
    initGame(&game);
    
    if(pthread_create(&threadClockID, NULL, &threadClock, &clkData)) {
        perror("Error creating threadClock\n");

        return -1;
    }
    printf("\nCreated thread Clock");

    createPipe(pipeBot,&game);
    
    kbData.game = &game;
    kbData.stop = 0;
    kbData.mutexGame = &mutexGame;
    
    
    if(pthread_create(&threadKBID, NULL, &threadKBEngine, &kbData)){
        perror("Error creating threadKB\n");
        return -1;
    }

    printf("\nCreated thread Keyboard");

    //call placePlayers when the game starts

    plData.game = &game;
    plData.stop = 0;
    plData.mutexGame = &mutexGame;

    if(pthread_create(&threadPlayersID, NULL, &threadPlayers, &plData)){
        perror("Error creating threadPlayers\n");
        return -1;
    } 
    
    pthread_join(threadACPID, NULL);
    pthread_join(threadClockID, NULL);
    pthread_join(threadKBID, NULL);
    pthread_join(threadPlayersID, NULL);
}

void *threadClock(void* data){
    CLKDATA *clkData = (CLKDATA *) data;
    printf("Inside threadClock\n");

    while(clkData->stop) {
        sleep(1);
        pthread_mutex_lock(clkData->mutexGame);
        if(clkData->game->timeleft > 0) 
            clkData->game->timeleft --;
        
        if(clkData->game->timeleft == 0 && clkData->game->level != 0) {
            //game over
        } else if(clkData->game->timeleft == 0 && clkData->game->level == 0){
            printf("Time to enroll has passed\n");
            passLevel(clkData->game);
        }
        pthread_mutex_unlock(clkData->mutexGame);

        //mutex unlock
    }
    printf("Outside threadClock\n");
    pthread_exit(NULL);
}

void *threadACP(void *data) {     //thread to accept players
    int flag = 0, i = 0,n, cont = 0;
    ACPDATA *acpData = (ACPDATA *) data;

    printf("\nInside thread ACP\n");

    int fdRdACP = open(FIFO_ENGINE_ACP, O_RDWR);
    if(fdRdACP == -1) {
        perror("Error opening FIFO for accepting players\n");
        //function to exit everything
    }

    while(acpData->stop){
        PLAYER player;
        flag = 0;
        char pipeName[50];
        
        int size = read(fdRdACP, &player, sizeof(PLAYER));
        if (size > 0) {
            printf("\n[PIPE %s] Player: %s\n",FIFO_ENGINE_ACP ,player.name);
        }
        
        
        pthread_mutex_lock(acpData->mutexGame);
        //sleep timeErolment
        if(acpData->game->nPlayers < 5 && acpData->game->level == 0 && acpData->game->start == 0 && acpData->timeEnrolment > 0) {
            for(i = 0 ; i < acpData->game->nPlayers ; i++) {
                if(strcmp(acpData->game->players[i].name, player.name) == 0) {
                    flag = 1;
                    player.accepted = 0;

                    printf("There is already a player with the same name\n");

                }
            }

            if(flag == 0) {
                player.accepted = 1;
                player.skin = acpData->game->nPlayers;
                acpData->game->players[acpData->game->nPlayers] = player;
                acpData->game->nPlayers++;

                printf("\nPlayer %s [%d] entered the game", player.name, acpData->game->nPlayers-1); 

            }
        }else{
            player.accepted = 2;
            acpData->game->nonPlayers[acpData->game->nNonPlayers] = player;
            acpData->game->nNonPlayers++;

            printf("\nThe number of players for this game has been reached or time for entry as ended\n");
        }
               
        pthread_mutex_unlock(acpData->mutexGame);

        //send player to gameui to see if he can enter the game
        printf("\n%d", player.pid);
        sprintf(pipeName, FIFO_GAMEUI, player.pid);
        printf("\n%s", pipeName);
        int fdWrInitGameui = open(pipeName, O_WRONLY);
        if (fdWrInitGameui == -1) {
            perror("open");
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeName, errno);
        }

        size = write (fdWrInitGameui, &player, sizeof(player));
        
        printf("Sent: %s with size [%d]\n", player.name, size);
        
    }
    printf("Outside threadACP\n");
    pthread_exit(NULL);
}

void setEnvVars() {
    printf("Created");
    setenv("ENROLLMENT", "60", 1); // Create the variable, overwriting if exist (1) 
    setenv("NPLAYERS", "2", 1); 
    setenv("DURATION", "60", 1); 
    setenv("DECREMENT", "5", 1); 
}

void getEnvVars(GAME *game, ACPDATA *acpData) {
    char *p = NULL;

    p = getenv("ENROLLMENT"); // Ler variável com um determinado nome

    if(p != NULL) {
        printf("\nVariable: ENROLLMENT = %s", p);
        acpData->timeEnrolment = (int) *p;
    }
    //cast to int to put in struct game

    p = getenv("NPLAYERS"); // Ler variável com um determinado nome

    if(p != NULL) {
        printf("\nVariable: NPLAYERS = %s", p);
        game->minNplayers = (int) *p;
    }

    p = getenv("DURATION"); // Ler variável com um determinado nome

    if(p != NULL) {
        printf("\nVariable: DURATION = %s", p);
        game->time = (int) *p;
    }

    p = getenv("DECREMENT"); // Ler variável com um determinado nome

    if(p != NULL) {
        printf("\nVariable: DECREMENT = %s\n", p);
        game->timeDec = (int) *p;
    }
}

void createPipe(int *pipeBot, GAME *game){
    pipe(pipeBot);
    game->pipeBot = pipeBot;
}

int launchBot(int *pipeBot, GAME *game){
    int pidBot = fork();
    printf("\nfork result %d\n", pidBot);

    if(pidBot <0){
        perror("Error creating Bot (Failed Fork())");
        return -1;
    }
    if (pidBot == 0) {
        if (dup2(pipeBot[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipeBot[1]);
        close(pipeBot[0]);

        int res = execl("bot/bot.exe", "bot", "2","2", NULL);
        if(res!=0){
            perror("Failed Execl ");
        }
    }
    else {
        printf("\nBack to the parent\n");
        game->nBots++;
        return pidBot;
    }

}

void readBot(int *pipeBot, int pid) {
    char buffer[BUFFER_SIZE];
    int bytes_read,i=0;
    printf("\npipebot %d\n", pipeBot[0]);
    printf("\npid do bot %d\n", pid);
        close(pipeBot[1]);
    while(i<3){
        // Read from the pipe
        bytes_read = read(pipeBot[0], buffer, 256);
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

void closeBot(int pid, GAME *game) {
    int status = 0;
    union sigval value;
    value.sival_int = 2;  // You can pass an integer value if needed

    if (sigqueue((pid_t)pid, SIGINT, value) == 0) {


        printf("Sent SIGINT signal to Bot with PID %d\n", pid);
        wait(&pid);
            if(WIFEXITED(pid)){
                printf("Bot with PID [%d] terminated (%d)\n",pid,status);
                game->nBots--;
            }
    } else {
        perror("sigqueue");
    }
} 

void initGame(GAME *game) {
    game->level = 0;
    game->nPlayers = 0;
    game->nBots = 0;
    game->nNonPlayers = 0;
    game->timeleft = 20;
}
 
void *threadKBEngine(void *data) {        //thread to receive commands from the kb
    KBDATA *kbData = (KBDATA *) data;
    char cmd[200], str1[30], str2[30];
    
    while(kbData->stop) {
        int flag = 0;
        printf("\nCommand: ");
        fflush(stdout);
        fgets(cmd, sizeof(cmd), stdin);
        printf("\n");
        
        
        if (sscanf(cmd, "%s %s", str1, str2) == 2 && strcmp(str1, "kick") == 0) {
            pthread_mutex_lock(kbData->mutexGame); 
            for(int i = 0 ; i < kbData->game->nPlayers ; i++) {
                if(strcmp(kbData->game->players[i].name, str2)) {
                    printf("\nPlayer %s has been kicked\n", kbData->game->players[i].name);
                    //send signal to player and remove him from the game
                    printf("\nVALID");
                    
                    flag = 1;
                    break;
                }
            }
            pthread_mutex_unlock(kbData->mutexGame); 
            if(flag == 0) {
                printf("\nThere is no player with the name %s\n", str2);
            } else {
                pthread_mutex_lock(kbData->mutexGame); 
                //kickPlayer();
                pthread_mutex_unlock(kbData->mutexGame); 
            }
        } else {
            cmd[ strlen( cmd ) - 1 ] = '\0';
            if(strcmp(cmd, "users") == 0) {
                pthread_mutex_lock(kbData->mutexGame); 
                printf("\nPlayers:\n");

                for(int i = 0 ; i < kbData->game->nPlayers ; i++) {
                    printf("\tPlayer %s\n", kbData->game->players[i].name);
                }

                pthread_mutex_unlock(kbData->mutexGame); 
            } 
            else if(strcmp(cmd, "bots") == 0) {
                pthread_mutex_lock(kbData->mutexGame); 
                if(kbData->game->nBots == 0) {
                    printf("\nThere are no bots yet\n");
                    break;
                }
                printf("\nBots:\n");
                for(int i = 0 ; i < kbData->game->nBots ; i++) {
                    printf("Bot %d with PID[%d]\t interval [%d]\t duration [%d]", 
                    i, kbData->game->bots[i].pid, kbData->game->bots[i].interval, kbData->game->bots[i].duration);
                }
                pthread_mutex_unlock(kbData->mutexGame); 
            } 
            else if(strcmp(cmd, "bmov") == 0) {
                pthread_mutex_lock(kbData->mutexGame); 


                pthread_mutex_unlock(kbData->mutexGame); 
            } 
            else if(strcmp(cmd, "rbm") == 0) {
                pthread_mutex_lock(kbData->mutexGame); 


                pthread_mutex_unlock(kbData->mutexGame); 

            } 
            else if(strcmp(cmd, "begin") == 0) {  //init the game automatically even if there is no min number of Players
                pthread_mutex_lock(kbData->mutexGame); 
                kbData->game->start = 1;
                passLevel(kbData->game);
                pthread_mutex_unlock(kbData->mutexGame); 
            } 
            /*
            else if(strcmp(cmd, "bot") == 0) {
                printf("\nVALID");
                kbData->game->bots[0].pid = launchBot(kbData->game->pipeBot, kbData->game);
                readBot(kbData->game->pipeBot, kbData->game->bots[0].pid);
                closeBot(kbData->game->bots[0].pid,  kbData->game);
                
            } 
            */
            else if(strcmp(cmd, "exit") == 0) {
                pthread_mutex_lock(kbData->mutexGame); 
                //avisar os bots com um sinal;
                //avisar os jogadores;
                pthread_mutex_unlock(kbData->mutexGame); 
            } 
            else {
                printf("\nINVALID");
            } 
        }
        
    }
    pthread_exit(NULL);
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

}

void movePlayer(GAME *game, PLAYER *player, DINAMICOBS *obstacle){    //meter esta func bem
    int flagWin = 0;
    int move;
    char skin;

    if(player == NULL) {  //dynamic obstacle
        srand(time(NULL));
        move = rand() % 4 + 1;
        skin = 'O';
    } else {
        move = player->move;
        skin = player->skin;
    }

    switch (move) {
        case 0:     //left
            if(game->map[player->position[0]][player->position[1] - 1] == ' ') {
                game->map[player->position[0]][player->position[1] - 1] == skin;
                game->map[player->position[0]][player->position[1]] == ' ';

                player->position[1] = player->position[1] - 1;
            }
            break;
        case 1:     //up
            if(game->map[player->position[0] - 1][player->position[1]] == ' ') {
                if(player->position[0 - 1] == 0) {
                    flagWin = 1;
                }

                game->map[player->position[0] - 1][player->position[1]] == skin;
                game->map[player->position[0]][player->position[1]] == ' ';

                player->position[0] = player->position[0] - 1;

                if(flagWin == 1) {
                    //call function to pass the level and initiate the game again
                }
            }
            break;
        case 2:     //right
            if(game->map[player->position[0]][player->position[1] + 1] == ' ') {
                game->map[player->position[0]][player->position[1] + 1] == skin;
                game->map[player->position[0]][player->position[1]] == ' ';

                player->position[1] = player->position[1] + 1;
            }
            break;
        case 3:     //down
            if(game->map[player->position[0] + 1][player->position[1]] == ' ') {
                game->map[player->position[0] + 1][player->position[1]] == skin;
                game->map[player->position[0]][player->position[1]] == ' ';

                player->position[0] = player->position[0] + 1;
            }
            break;
    }
}

void *threadPlayers(void *data) {
    PLAYERSDATA *plData = (PLAYERSDATA *) data;
    int size = 0;
    PLAYER player;
    MESSAGE msg;

    printf("\nInside thread Players\n");
    if(mkfifo(FIFO_ENGINE_GAME, 0666) == -1) {
        perror("Error creating fifo engine game\n"); 
    }  
    int fdRdPlayerMoves = open(FIFO_ENGINE_GAME, O_RDWR);
	if(fdRdPlayerMoves == -1) {
		perror("Error openning fifo engine game\n"); 
	}
    char pipeNameGamePlayer[30];
    while(plData->stop == 0) {  //aqui tem de se meter == 0
        //read the player ENGINE FIFO GAME
        size = read (fdRdPlayerMoves, &player, sizeof(PLAYER));
        printf("\nReceived player: %s with move: %d\n", player.name, player.move);

        if(player.move == -2) {     //player sent a msg
            pthread_mutex_lock(plData->mutexGame); 
            for(int i = 0 ; i < plData->game->nPlayers; i++) {
                if(strcmp(player.personNameMessage, plData->game->players[i].name) == 0) {
                    sprintf(msg.pipeName, FIFO_PRIVATE_MSG, plData->game->players[i].pid);
                    //send to the player the struct message
                    char pipeNamePIDPlayer[30];
                    sprintf(pipeNamePIDPlayer, FIFO_PRIVATE_MSG, player.pid);

                    int fdWrPIDPlayer = open(pipeNamePIDPlayer, O_RDWR);
                    if(fdWrPIDPlayer == -1) {
                        perror("Error openning pid message fifo\n"); 
                    }

                    size = write (fdWrPIDPlayer, &msg, sizeof(MESSAGE));

                    break;
                }
            }
            pthread_mutex_unlock(plData->mutexGame); 
            
        } else if(player.move == -1) {     //player sent a command
            if(strcmp(player.command, "players") == 0) {
                pthread_mutex_lock(plData->mutexGame); 

                for(int i = 0 ; i < plData->game->nPlayers ; i++) {
                    strcpy(player.players[i].name, plData->game->players[i].name);
                }

                pthread_mutex_unlock(plData->mutexGame); 
                //no gameui fazer na thread que tem acesso à janela onde vao aparecer os players
                //se o array de players do player estiver preenchido então vai dar print dos nomes
            } else if(strcmp(player.command, "exit") == 0) {
                pthread_mutex_lock(plData->mutexGame); 
                //kickPlayer();
                pthread_mutex_unlock(plData->mutexGame); 
            }
        } else { //move normal
            //send to function the player from game (game.player[i])
            pthread_mutex_lock(plData->mutexGame);  //ver se se mete aqui o mutex ou mesmo lá dentro
            for(int i = 0 ; i < plData->game->nPlayers ; i++) {
                if(strcmp(plData->game->players[i].name, player.name) == 0) {
                    movePlayer(plData->game, &plData->game->players[i], NULL);
                    break;
                }
            }
            pthread_mutex_unlock(plData->mutexGame); 
            //send to all players the new game -> passar function
            
            pthread_mutex_lock(plData->mutexGame); 
            for(int i = 0 ; i < plData->game->nPlayers ; i++) {
                sprintf(pipeNameGamePlayer, FIFO_GAMEUI, plData->game->players[i].pid);

                int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
                if(fdWrGamePlayer == -1) {
                    perror("Error openning game fifo\n"); 
                }

                size = write (fdWrGamePlayer, plData->game, sizeof(GAME));
            }
            for(int i = 0 ; i < plData->game->nNonPlayers ; i++) {
                sprintf(pipeNameGamePlayer, FIFO_GAMEUI, plData->game->nonPlayers[i].pid);

                int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
                if(fdWrGamePlayer == -1) {
                    perror("Error openning game fifo\n"); 
                }

                size = write (fdWrGamePlayer, plData->game, sizeof(GAME));
            }
            pthread_mutex_unlock(plData->mutexGame); 
        }
    }

    pthread_exit(NULL);
}

void placePlayers(GAME *game) {
    int colRand = rand() % 39 + 1;

    for(int i = 0 ; i < game->nPlayers ; i++) {
        srand(time(NULL));
        colRand = rand() % 39 + 1;
        if(game->map[14][colRand] == ' ') {
            game->map[14][colRand] = game->players[i].skin;
            game->players[i].position[0] = 14;
            game->players[i].position[1] = colRand;
        }
    }
}

void passLevel(GAME *game) {
    game->start = 1;
    game->level += 1;
    readFileMap(game->level, game);
    placePlayers(game);
    game->time = game->time - game->timeDec;
    game->timeleft = game->time;
}