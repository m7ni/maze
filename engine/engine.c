#include "engine.h"

int stop;

int main(int argc, char *argv[])
{
    ACPDATA acpData;
    KBDATA kbData;
    PLAYERSDATA plData;
    CLKDATA clkData;
    TBDATA tbData;
    GAME game;
    int pipeBot[2], timeEnrolment;
    pthread_t threadACPID, threadKBID, threadPlayersID, threadClockID, threadBotID;
    pthread_mutex_t mutexGame;
    stop = 0;

    // Creating FIFO for accepting PLAYERS
    if (mkfifo(FIFO_ENGINE_ACP, 0666) == -1)
    {
        perror("Error opening fifo or another engine is already running\n");
        return -1;
    }

    struct sigaction sa;
    sa.sa_sigaction = handlerSignalEngine;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR2, &sa, NULL);

    setEnvVars();
    initGame(&game);
    getEnvVars(&game, &acpData);

    pthread_mutex_init(&mutexGame, NULL);

    acpData.game = &game;
    acpData.stop = stop;
    acpData.mutexGame = &mutexGame;

    if (pthread_create(&threadACPID, NULL, &threadACP, &acpData))
    {
        perror("Error creating threadACP\n");
        return -1;
    }
    printf("\nCreated thread ACP");

    clkData.game = &game;
    clkData.stop = stop;
    clkData.mutexGame = &mutexGame;

    if (pthread_create(&threadClockID, NULL, &threadClock, &clkData))
    {
        perror("Error creating threadClock\n");

        return -1;
    }
    printf("Created thread Clock\n");

    createPipe(pipeBot, &game);

    kbData.game = &game;
    kbData.stop = stop;
    kbData.mutexGame = &mutexGame;

    if (pthread_create(&threadKBID, NULL, &threadKBEngine, &kbData))
    {
        perror("Error creating threadKB\n");
        return -1;
    }

    printf("\nCreated thread Keyboard");

    // call placePlayers when the game starts

    plData.game = &game;
    plData.stop = stop;
    plData.mutexGame = &mutexGame;

    if (pthread_create(&threadPlayersID, NULL, &threadPlayers, &plData))
    {
        perror("Error creating threadPlayers\n");
        return -1;
    }

    tbData.game = &game;
    tbData.stop = stop;
    tbData.mutexGame = &mutexGame;

    if (pthread_create(&threadBotID, NULL, &threadReadBot, &tbData))
    {
        perror("Error creating threadPlayers\n");
        return -1;
    }

    while(1) {
        if(stop == 1) {
            pthread_kill(threadACPID, SIGUSR2);
            pthread_kill(threadClockID, SIGUSR2);
            pthread_kill(threadKBID, SIGUSR2);
            pthread_kill(threadPlayersID, SIGUSR2);
            pthread_kill(threadBotID, SIGUSR2);
            break;
        }
    }

    pthread_join(threadACPID, NULL);
    pthread_join(threadClockID, NULL);
    pthread_join(threadKBID, NULL);
    pthread_join(threadPlayersID, NULL);
    pthread_join(threadBotID, NULL);
}

void *threadReadBot(void *data)
{
    TBDATA *tbData = (TBDATA *)data;
    char buffer[BUFFER_SIZE];
    char col[20], lin[20], duration[20];
    int bytes_read, i = 0;
    // printf("\npipebot %d\n", tbpipeBot[0]);
    // printf("\npid do bot %d\n", pid);
    // close(pipeBot[1]);

    pthread_mutex_lock(tbData->mutexGame);
    for (int i = 0; i < 50; i++)
    {
        tbData->game->rocks[i].skin = 'U';
    }
    pthread_mutex_unlock(tbData->mutexGame);

    while (stop == 0)
    {

        // Read from the pipe
        bytes_read = read(tbData->game->pipeBot[0], buffer, 256);

        if (bytes_read < 0)
        {
            perror("read");
        }
        else
        {

            buffer[bytes_read] = '\0'; // Null-terminate the received data
            sscanf(buffer, "%s %s %s", col, lin, duration);
            int colN = atoi(col);
            int linN = atoi(lin);
            int durationN = atoi(duration);
            pthread_mutex_lock(tbData->mutexGame);

            placeRock(colN, linN, durationN, tbData->game);
            pthread_mutex_unlock(tbData->mutexGame);

            // Print the received string
            printf("Received: %s %s %s %d %d %d\n", lin, col, duration, linN, colN, durationN);
        }
    }
    printf("Outside threadBOT\n");
    pthread_exit(NULL);
}

void placeRock(int col, int lin, int duration, GAME *game)
{
    ROCK rock;

    if (game->map[lin][col] == ' ')
    {
        rock.duration = duration;
        rock.position[0] = lin;
        rock.position[1] = col;
        rock.skin = 'R';
        game->nRocks++;
        game->map[lin][col] = rock.skin;
        for (int i = 0; i < 50; i++)
        {
            if (game->rocks[i].skin == 'U')
            {
                game->rocks[i] = rock;
                break;
            }
        }
    }
    else
    {
        printf("\nSpace for Rock was occupied [%c] %d %d\n", game->map[lin][col], lin, col);
    }
}

void *threadClock(void *data)
{
    CLKDATA *clkData = (CLKDATA *)data;

    int size = 0;

    while (stop == 0)
    {
        sleep(1);
        pthread_mutex_lock(clkData->mutexGame);
        //printf("Timeleft: %d\n", clkData->game->timeleft);
        if (clkData->game->timeleft > 0)
        {
            clkData->game->timeleft--;

            for (int i = 0; i < 50; i++)
            {
                if (clkData->game->rocks[i].skin != 'U')
                {
                    if (clkData->game->rocks[i].duration > 0)
                    {
                        clkData->game->rocks[i].duration--;
                    }
                    else
                    {
                        clkData->game->rocks[i].skin = 'U';
                        clkData->game->map[clkData->game->rocks[i].position[0]][clkData->game->rocks[i].position[1]] = ' ';
                        clkData->game->nRocks--;
                        sendMap(clkData->game);
                    }
                }
            }

            for (int i = 0; i < clkData->game->nObs; i++)
            {
                movePlayer(clkData->game, NULL, &clkData->game->obstacle[i]);
                sendMap(clkData->game);
            }
        }

        if (clkData->game->timeleft == 0 && clkData->game->level != 0)
        {
            // game over
            printf("\nGame over");
        }
        else if (clkData->game->timeleft == 0 && clkData->game->level == 0 && clkData->game->nPlayers >= clkData->game->minNplayers|| clkData->game->start == 1)
        {

            if (clkData->game->start != 1)
            {
                printf("Time to enroll has passed\n");
                passLevel(clkData->game);
            }
            sendMap(clkData->game);
        }
        pthread_mutex_unlock(clkData->mutexGame);
    }
    printf("Outside threadClock\n");
    pthread_exit(NULL);
}

void sendMap(GAME *game) {
    int size = 0;
    char pipeNameGamePlayer[30];
    for (int i = 0; i < game->nPlayers; i++)
    {
        sprintf(pipeNameGamePlayer, FIFO_GAMEUI, game->players[i].pid);

        int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
        if (fdWrGamePlayer == -1)
        {
            printf("\nErro no open fifo Player: %s", game->players[i].name);
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeNameGamePlayer, errno);

            kickPlayer(game,game->players[i],1);
            perror("Error openning game fifo\n");
        }

        size = write(fdWrGamePlayer, game, sizeof(GAME));
    }
    for (int i = 0; i < game->nNonPlayers; i++)
    {
        sprintf(pipeNameGamePlayer, FIFO_GAMEUI, game->nonPlayers[i].pid);

        int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
        if (fdWrGamePlayer == -1)
        {
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeNameGamePlayer, errno);

            kickPlayer(game,game->nonPlayers[i],0);

            perror("Error openning game fifo\n");
        }

        size = write(fdWrGamePlayer, game, sizeof(GAME));
    }
}

void *threadACP(void *data)
{ // thread to accept players
    int flag = 0, i = 0, n, cont = 0;
    ACPDATA *acpData = (ACPDATA *)data;



    int fdRdACP = open(FIFO_ENGINE_ACP, O_RDWR);
    if (fdRdACP == -1)
    {
        perror("Error opening FIFO for accepting players\n");
        // function to exit everything
    }

    while (stop == 0)
    {
        PLAYER player;
        flag = 0;
        char pipeName[50];

        int size = read(fdRdACP, &player, sizeof(PLAYER));
        if (size > 0)
        {
            //printf("\n[PIPE %s] Player: %s\n", FIFO_ENGINE_ACP, player.name);
        }

        if(stop==1){
            break;
        }

        pthread_mutex_lock(acpData->mutexGame);
        /*
        printf("acpData->game->nPlayers %d\n",acpData->game->nPlayers);
        printf("acpData->game->level %d\n",acpData->game->level);
        printf("acpData->game->start %d\n",acpData->game->start);
        printf("acpData->timeEnrolment %d\n",acpData->timeEnrolment);*/


        if (acpData->game->nPlayers < 5 && acpData->game->level == 0 && acpData->game->start == 0 && acpData->timeEnrolment > 0)
        {
            for (i = 0; i < acpData->game->nPlayers; i++)
            {
                if (strcmp(acpData->game->players[i].name, player.name) == 0)
                {
                    flag = 1;
                    player.accepted = 0;

                    printf("There is already a player with the same name\n");
                }
            }

            if (flag == 0)
            {
                player.accepted = 1;
                switch (acpData->game->nPlayers)
                {
                case 0:
                    player.skin = '0';
                    break;
                case 1:
                    player.skin = '1';
                    break;
                case 2:
                    player.skin = '2';
                    break;
                case 3:
                    player.skin = '3';
                    break;
                case 4:
                    player.skin = '4';
                    break;
                }
                player.score = 0;
                acpData->game->players[acpData->game->nPlayers] = player;
                acpData->game->nPlayers++;

                printf("\nPlayer %s [%d] entered the game", player.name, acpData->game->nPlayers - 1);
            }
        }
        else
        {
            player.accepted = 2;
            acpData->game->nonPlayers[acpData->game->nNonPlayers] = player;
            acpData->game->nNonPlayers++;

            printf("\nThe number of players for this game has been reached or time for entry as ended\n");
        }

        pthread_mutex_unlock(acpData->mutexGame);

        // send player to gameui to see if he can enter the game

        sprintf(pipeName, FIFO_GAMEUI, player.pid);

        int fdWrInitGameui = open(pipeName, O_WRONLY);
        if (fdWrInitGameui == -1)
        {
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeName, errno);
        }

        size = write(fdWrInitGameui, &player, sizeof(player));

        //printf("Sent: %s with size [%d]\n", player.name, size);
    }
    
    printf("Outside threadACP\n");
    unlink(FIFO_ENGINE_ACP);
    close(fdRdACP);
    pthread_exit(NULL);
}

void setEnvVars()
{
    setenv("ENROLLMENT", "60", 1); // Create the variable, overwriting if exist (1)
    setenv("NPLAYERS", "2", 1);
    setenv("DURATION", "60", 1);
    setenv("DECREMENT", "5", 1);
}

void getEnvVars(GAME *game, ACPDATA *acpData)
{
    char *p = NULL;

    p = getenv("ENROLLMENT"); // Ler variável com um determinado nome

    if (p != NULL)
    {
        printf("\nVariable: ENROLLMENT = %s", p);
        acpData->timeEnrolment = (int)*p;
        game->timeleft = acpData->timeEnrolment;
    }
    // cast to int to put in struct game

    p = getenv("NPLAYERS"); // Ler variável com um determinado nome

    if (p != NULL)
    {
        printf("\nVariable: NPLAYERS = %s", p);
        game->minNplayers = (int)*p;
    }

    p = getenv("DURATION"); // Ler variável com um determinado nome

    if (p != NULL)
    {
        printf("\nVariable: DURATION = %s", p);
        game->time = (int)*p;
    }

    p = getenv("DECREMENT"); // Ler variável com um determinado nome

    if (p != NULL)
    {
        printf("\nVariable: DECREMENT = %s\n", p);
        game->timeDec = atoi(p);
        // game->timeDec = 5;
    }
}

void createPipe(int *pipeBot, GAME *game)
{
    pipe(pipeBot);
    game->pipeBot = pipeBot;
}

void launchBot(GAME *game)
{
    int coord = 30;
    char coordS[20];
    int duration;
    char durationS[20];

    switch (game->level)
    {
    case 1:
        duration = 10;
        break;
    case 2:
        duration = 15;
        break;
    case 3:
        duration = 20;
        break;
    }

    for (int i = 0; i < game->level + 1; i++)
    {
        BOT bot;
        int pidBot = fork();
        printf("\nfork result %d\n", pidBot);

        sprintf(coordS, "%d", coord);
        sprintf(durationS, "%d", duration);

        printf("\ndurationS [%s]\t", durationS);
        printf("CoordS [%s]\n", coordS);

        if (pidBot < 0)
        {
            perror("Error creating Bot (Failed Fork())\n");
            continue;
        }
        if (pidBot == 0)
        {
            printf("\nbot [%d] with pid %d\n", i, getpid());
            if (dup2(game->pipeBot[1], STDOUT_FILENO) == -1)
            {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(game->pipeBot[1]);
            close(game->pipeBot[0]);
            int res = execl("bot/bot.exe", "bot", coordS, durationS, NULL);
            if (res != 0)
            {
                perror("Failed Execl\n");
            }
        }
        else
        {
            bot.pid = pidBot;
            printf("PARENT bot [%d] with pid %d\n", i, bot.pid);
            bot.duration = coord;
            bot.interval = duration;
            game->bots[game->nBots++] = bot;
            coord = coord - 5;
            duration = duration - 5;

        }
    }
}

void closeBot(GAME *game)
{
    int status = 0;
    union sigval value;
    value.sival_int = 2; // You can pass an integer value if needed

    for (int i = 0; i < game->nBots; i++)
    {
        printf("Close Bot [%d][%d]\n", game->nBots, game->bots[i].pid);
        if (sigqueue((pid_t)game->bots[i].pid, SIGINT, value) == 0)
        {
            //printf("Sent SIGINT signal to Bot with PID %d\n", game->bots[i].pid);
            wait(&game->bots[game->nBots].pid);
            if (WIFEXITED(game->bots[i].pid))
            {
                //printf("Bot with PID [%d] terminated (%d)\n", game->bots[i].pid, status);
                game->nBots--;
            }
        }
        else
        {
            perror("sigqueue");
        }
    }
}

void initGame(GAME *game)
{
    game->level = 0;
    game->nPlayers = 0;
    game->nBots = 0;
    game->nNonPlayers = 0;
    game->nObs = 0;
    game->nRocks = 0;
    game->start = 0;

    // game->timeleft = 10;
}

void *threadKBEngine(void *data)
{ // thread to receive commands from the kb
    KBDATA *kbData = (KBDATA *)data;
    char cmd[200], str1[30], str2[30];

    while (stop == 0)
    {
        int flag = 0;
        printf("\nCommand: ");
        fflush(stdout);
        fgets(cmd, sizeof(cmd), stdin);
        printf("\n");

        if (sscanf(cmd, "%s %s", str1, str2) == 2 && strcmp(str1, "kick") == 0) //POR IMPLEMENTAR
        {
            pthread_mutex_lock(kbData->mutexGame);
            for (int i = 0; i < kbData->game->nPlayers; i++)
            {                
                if (strcmp(kbData->game->players[i].name, str2) == 0)
                {
                    kickPlayer(kbData->game, kbData->game->players[i], 1);
                    
                    flag = 1;
                    break;
                }
            }
            for (int i = 0; i < kbData->game->nNonPlayers; i++)
            {                
                if (strcmp(kbData->game->nonPlayers[i].name, str2) == 0)
                {
                    kickPlayer(kbData->game, kbData->game->nonPlayers[i], 1);
                    
                    flag = 1;
                    break;
                }
            }
            pthread_mutex_unlock(kbData->mutexGame);
            if (flag == 0)
            {
                printf("\nThere is no player with the name %s\n", str2);
            }
        }
        else
        {
            cmd[strlen(cmd) - 1] = '\0';
            if(kbData->game->start == 0) {
                if (strcmp(cmd, "begin") == 0) 
                { // init the game automatically even if there is no min number of Players
                    pthread_mutex_lock(kbData->mutexGame);
                    kbData->game->start = 1;
                    passLevel(kbData->game);
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "exit") == 0)
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    for(int i = 0 ; i < 5; i++) {
                        // avisar os jogadores;
                        kickPlayer(kbData->game, kbData->game->players[0], 1);
                        
                    }
                    for(int i = 0 ; i < 5 ; i++) {
                        // avisar os nao jogadores;
                        kickPlayer(kbData->game, kbData->game->nonPlayers[0], 0);
                    }
                    // avisar os bots com um sinal;
                    for(int i = 0 ; i < kbData->game->nBots ; i++) {
                        closeBot(kbData->game);
                    }

                    union sigval val;
                    val.sival_int = 4;        //linha que acrescenta para o exemplo 2

                    sigqueue(getpid(), SIGUSR2, val);
                    
                    pthread_mutex_unlock(kbData->mutexGame);
                }
            } else {
                if (strcmp(cmd, "users") == 0) 
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    printf("\nPlayers:\n");

                    for (int i = 0; i < kbData->game->nPlayers; i++)
                    {
                        printf("\tPlayer %s\n", kbData->game->players[i].name);
                    }

                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "bots") == 0) 
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    if (kbData->game->nBots == 0)
                    {
                        printf("\nThere are no bots yet\n");
                        break;
                    }
                    printf("\nBots:\n");
                    for (int i = 0; i < kbData->game->nBots; i++)
                    {
                        printf("Bot %d with PID[%d]\t interval [%d]\t duration [%d]\n",
                            i, kbData->game->bots[i].pid, kbData->game->bots[i].interval, kbData->game->bots[i].duration);
                    }
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "bmov") == 0) 
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    if (kbData->game->nBots == 20)
                    {
                        printf("Maximum number of obstacles allowed reached");
                    }
                    else
                    {
                        insertDynamicObstacle(kbData->game);
                    }
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "rbm") == 0) 
                {
                    pthread_mutex_lock(kbData->mutexGame);

                    if (kbData->game->nObs > 0)
                    {
                        removeDynamicObstacle(kbData->game);
                    }
                    else
                    {
                        printf("No obstacles left to remove");
                    }
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "begin") == 0) 
                { 


                    pthread_mutex_lock(kbData->mutexGame);
                    kbData->game->start = 1;
                    passLevel(kbData->game);
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "exit") == 0) 
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    for(int i = 0 ; i < 5; i++) {
                        kickPlayer(kbData->game, kbData->game->players[0], 1);
                        
                    }
                    for(int i = 0 ; i < 5 ; i++) {
                        kickPlayer(kbData->game, kbData->game->nonPlayers[0], 0);
                    }
                    for(int i = 0 ; i < kbData->game->nBots ; i++) {
                        closeBot(kbData->game);
                    }

                    union sigval val;
                    val.sival_int = 4;        //linha que acrescenta para o exemplo 2

                    sigqueue(getpid(), SIGUSR2, val);
                    
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else
                {
                    printf("\nINVALID");
                }
            }
            
        }
    }
    printf("\nOutside threadKB");
    pthread_exit(NULL);
}

void removeDynamicObstacle(GAME *game)
{
    for (int i = 0; i < game->nObs - 1; i++)
    {
        if(i==0){
            printf("Lin: %d Col: %d\n", game->obstacle[i].position[0], game->obstacle[i].position[1]);
            game->map[game->obstacle[i].position[0]][game->obstacle[i].position[1]] = ' ';
            sendMap(game);
        }
        game->obstacle[i] = game->obstacle[i + 1];
    }
    game->nObs--;
    printf("\nnObs %d\n", game->nObs);
}

void insertDynamicObstacle(GAME *game)
{
    DINAMICOBS obs;
    obs.skin = 'W';
    int x, y;
    while (1)
    {
        srand(time(NULL));
        x = rand() % NCOL;
        y = rand() % NLIN;
        if (game->map[y][x] == ' ')
        {
            game->map[y][x] = 'O';
            obs.position[0] = y;
            obs.position[1] = x;
            game->obstacle[game->nObs++] = obs;
            break;
        }
    }
}

void readFileMap(int level, GAME *game)
{
    FILE *file;
    char fileName[30];

    switch (level)
    {
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
    if (file == NULL)
    {
        perror("\nError opening file\n");
        return; // Return an error code
    }

    // Read the content of the file into the array
    for (int i = 0; i < 16; ++i)
    {
        for (int j = 0; j < 40; ++j)
        {
            int ch = fgetc(file);

            // Check for end of file or error
            if (ch == EOF || ch == '\n')
            {
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
    for (int i = 0; i < 16; ++i)
    {
        for (int j = 0; j < 40; ++j)
        {
            printf("%c", game->map[i][j]);
        }
        printf("\n");
    }
}

void movePlayer(GAME *game, PLAYER *player, DINAMICOBS *obstacle)
{ // meter esta func bem
    int flagWin = 0;
    int move;
    char skin;
    int lin;
    int col;
    if (player == NULL)
    { // dynamic obstacle
        srand(time(NULL));
        move = rand() % 4 + 1;
        skin = obstacle->skin;
        lin = obstacle->position[0];
        col = obstacle->position[1];
    }
    else
    {
        move = player->move;
        skin = player->skin;
        lin = player->position[0];
        col = player->position[1];
    }

    switch (move)
    {
    case 0: // left
        if (game->map[lin][col - 1] == ' ')
        {
            game->map[lin][col - 1] = skin;
            game->map[lin][col] = ' ';

            col = col - 1;
        }
        break;
    case 1: // up
        if (game->map[lin - 1][col] == ' ')
        {
            if (lin - 1 == 0 && player != NULL)
            {
                flagWin = 1;
            }

            game->map[lin - 1][col] = skin;
            game->map[lin][col] = ' ';
            lin = lin - 1;

            if (flagWin == 1)
            {
                // call function to pass the level and initiate the game again
                player->score++;
                passLevel(game);
            }
        }
        break;
    case 2: // right
        if (game->map[lin][col + 1] == ' ')
        {
            game->map[lin][col + 1] = skin;
            game->map[lin][col] = ' ';

            col = col + 1;
        }
        break;
    case 3: // down
        if (game->map[lin + 1][col] == ' ')
        {
            game->map[lin + 1][col] = skin;
            game->map[lin][col] = ' ';

            lin = lin + 1;
        }
        break;
    }

    if (player == NULL)
    { // dynamic obstacle
        
        obstacle->position[0] = lin;
        obstacle->position[1] = col;
    }
    else if(player != NULL && flagWin == 0)
    {
        player->position[0] = lin;
        player->position[1] = col;
    }
    flagWin = 0;

}

void *threadPlayers(void *data)
{
    PLAYERSDATA *plData = (PLAYERSDATA *)data;
    int size = 0;
    PLAYER player;
    MESSAGE msg;
    int flag = 1;

    printf("\nInside thread Players\n");
    if (mkfifo(FIFO_ENGINE_GAME, 0666) == -1)
    {
        perror("Error creating fifo engine game\n");
    }
    int fdRdPlayerMoves = open(FIFO_ENGINE_GAME, O_RDWR);
    if (fdRdPlayerMoves == -1)
    {
        perror("Error openning fifo engine game\n");
    }
    char pipeNameGamePlayer[30];
    while (stop == 0)
    { 
        // read the player ENGINE FIFO GAME
        size = read(fdRdPlayerMoves, &player, sizeof(PLAYER));
        printf("\nReceived player: %s with move: %d\n", player.name, player.move);
        int indice;
        if (player.skin == '0')
        {
            indice = 0;
        }
        else if (player.skin == '1')
        {
            indice = 1;
        }
        else if (player.skin == '2')
        {
            indice = 2;
        }
        else if (player.skin == '3')
        {
            indice = 3;
        }
        else if (player.skin == '4')
        {
            indice = 4;
        }
        plData->game->players[indice].move = player.move;

        if (player.move == -2)
        { // player sent a msg
            pthread_mutex_lock(plData->mutexGame);
            for (int i = 0; i < plData->game->nPlayers + 1; i++)
            {
                if (strcmp(player.personNameMessage, plData->game->players[i].name) == 0)
                {
                    sprintf(msg.pipeName, FIFO_PRIVATE_MSG, plData->game->players[i].pid);

                    // send to the player the struct message
                    char pipeNamePIDPlayer[30];
                    sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, player.pid);


                    strcpy(plData->game->players[indice].personNameMessage, player.name);
                    strcpy(plData->game->players[indice].message, player.message);

                    int fdWrPIDPlayer = open(pipeNamePIDPlayer, O_WRONLY);

                    if (fdWrPIDPlayer == -1)
                    {
                        perror("Error openning pid message fifo\n");
                    }
                    size = write(fdWrPIDPlayer, &msg, sizeof(MESSAGE));
                    close(fdWrPIDPlayer);

                    flag = 0;
                    break;
                }
                flag = 1;
            }

            if (flag == 1)
            {
                char pipeNamePIDPlayer[30];
                sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, player.pid);

                int fdWrPIDPlayer = open(pipeNamePIDPlayer, O_RDWR);
                if (fdWrPIDPlayer == -1)
                {
                    perror("Error openning pid message fifo\n");
                }

                strcpy(msg.pipeName, "error");

                size = write(fdWrPIDPlayer, &msg, sizeof(MESSAGE));

                close(fdWrPIDPlayer);
            }
            pthread_mutex_unlock(plData->mutexGame);
        }
        else if (player.move == -1)
        { // player sent a command
            if (strcmp(player.command, "players") == 0)
            {
                pthread_mutex_lock(plData->mutexGame);

                for (int i = 0; i < plData->game->nPlayers; i++)
                {
                    strcpy(player.players[i].name, plData->game->players[i].name);
                }

                pthread_mutex_unlock(plData->mutexGame);
                // no gameui fazer na thread que tem acesso à janela onde vao aparecer os players
                // se o array de players do player estiver preenchido então vai dar print dos nomes
            }
            
        }
        else if (player.move == -3)
        {
            pthread_mutex_lock(plData->mutexGame);
            kickPlayer(plData->game, player, 1);
            pthread_mutex_unlock(plData->mutexGame);
        }
        else
        { // move normal
            // send to function the player from game (game.player[i])
            // pthread_mutex_lock(plData->mutexGame);  //ver se se mete aqui o mutex ou mesmo lá dentro
            for (int i = 0; i < plData->game->nPlayers; i++)
            {
                if (strcmp(plData->game->players[i].name, player.name) == 0)
                {
                    movePlayer(plData->game, &plData->game->players[i], NULL);
                    break;
                }
            }
            // pthread_mutex_unlock(plData->mutexGame);
            // send to all players the new game -> passar function

            pthread_mutex_lock(plData->mutexGame);
            sendMap(plData->game);
            pthread_mutex_unlock(plData->mutexGame);
        }
    }

    unlink(FIFO_ENGINE_GAME);
    close(fdRdPlayerMoves);
    printf("\nOutside threadPlayer");
    pthread_exit(NULL);
}

void placePlayers(GAME *game)
{
    int colRand = rand() % 40 + 1;
    for (int i = 0; i < game->nPlayers; i++)
    {
        do
        {
            srand(time(NULL));
            colRand = rand() % 40 + 1;
            if (game->map[14][colRand] == ' ')
            {
                game->map[14][colRand] = game->players[i].skin;
                game->players[i].position[0] = 14;
                game->players[i].position[1] = colRand;
                break;
            }
        } while (1);

        printf("\nSkin: %c, Position [%d][%d]\n", game->players[i].skin, game->players[i].position[0], game->players[i].position[1]);
    }
}

void passLevel(GAME *game)
{
    if(game->level == 3) {
        //end engine
        
        return;
    }
    game->start = 1;
    printf("\nGame level: %d\n", game->level);
    if (game->level == 0) {
        game->time = 105; // time of level 1
    }
    game->level += 1;
    readFileMap(game->level, game);
    placePlayers(game);
    closeBot(game);
    launchBot(game);

    game->time = game->time - game->timeDec;
    game->timeleft = game->time;

    printf("Time of the level: %d\n", game->time);
    printf("Timedec: %d\n", game->timeDec);
    printf("TimeLeft: %d\n", game->timeleft);
}

void kickPlayer(GAME *game, PLAYER player, int accepted) {
    union sigval val;
    val.sival_int = 3;
    
    // send signal to player and remove him from the game
    if(accepted == 0) {
        for (int i = 0; i < game->nNonPlayers; i++)
        {
            if(game->nonPlayers[i].pid == player.pid){
                game->map[game->nonPlayers[i].position[0]][game->nonPlayers[i].position[1]] = ' ';
                printf("\nNonPlayer %s has been kicked\n", game->nonPlayers[i].name);
                sigqueue(player.pid, SIGUSR1, val);
                
                printf("\nnNonPlayers %d\n", game->nNonPlayers);
                if(i != 4) {
                    for(int j = i ; j < game->nNonPlayers - 1; j++) {
                        game->nonPlayers[j] = game->nonPlayers[j + 1];
                    }
                }
                
                game->nNonPlayers -= 1;
                break;
            }
            
        }
        
        sendMap(game);
    } else {
        for (int i = 0; i < game->nPlayers; i++)
        {
            if(game->players[i].pid == player.pid){
                game->map[game->players[i].position[0]][game->players[i].position[1]] = ' ';
                printf("\nPlayer %s has been kicked\n", game->players[i].name);
                sigqueue(player.pid, SIGUSR1, val);
                
                printf("\nnPlayers %d\n", game->nPlayers);
                if(i != 4) {
                    for(int j = i ; j < game->nPlayers - 1; j++) {
                        game->players[j] = game->players[j + 1];
                    }
                }

                game->nPlayers -= 1;
                break;
            }
            
        }
        sendMap(game);
    }
    
}

void handlerSignalEngine(int signum, siginfo_t *info, void *secret) {
	stop = 1;
}