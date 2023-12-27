#include "engine.h"

int stop;

/**
 * The main function of the game engine program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return Returns 0 upon successful execution.
 */
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
    if (mkfifo(FIFO_ENGINE_ACP, 0666) == -1) {
        printf("Error opening fifo or another engine is already running\n");
        return -1;
    }

    // Struct for SIGUSR1, responsible initiating the orderly end of the process
    struct sigaction sa_usr2;
    sa_usr2.sa_handler = handlerSignalEngine;
    sigemptyset(&sa_usr2.sa_mask);
    sa_usr2.sa_flags = SA_SIGINFO;

    // Struct for SIGINT, responsible for handling CTRL+C signal
	struct sigaction sa_int;
    sa_int.sa_handler = handlerSignalEngine;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        printf("Unable to register SIGINT signal handler\n");
    }

	if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1) {
        printf("Unable to register SIGUSR2 signal handler\n");
    }

    setEnvVars();   // Set environment variables
    initGame(&game);
    getEnvVars(&game, &acpData); // Reading environment variables

    pthread_mutex_init(&mutexGame, NULL);

    acpData.game = &game;
    acpData.stop = stop;
    acpData.mutexGame = &mutexGame;

    // Opening thread responsible for acepting Players into the game
    if (pthread_create(&threadACPID, NULL, &threadACP, &acpData)){
        printf("Error creating threadACP\n");
        return -1;
    }

    clkData.game = &game;
    clkData.stop = stop;
    clkData.mutexGame = &mutexGame;

    // Opening thread responsible for time management
    if (pthread_create(&threadClockID, NULL, &threadClock, &clkData)){
        printf("Error creating threadClock\n");
        return -1;
    }


    createPipe(pipeBot, &game); // Creating Pipe redirecting for reading bot stdout

    kbData.game = &game;
    kbData.stop = stop;
    kbData.mutexGame = &mutexGame;

    // Opening thread responsible for handling Admin input
    if (pthread_create(&threadKBID, NULL, &threadKBEngine, &kbData)){
        printf("Error creating threadKB\n");
        return -1;
    }

    plData.game = &game;
    plData.stop = stop;
    plData.mutexGame = &mutexGame;

    // Opening thread responsible for handling Player input
    if (pthread_create(&threadPlayersID, NULL, &threadPlayers, &plData)){
        printf("Error creating threadPlayers\n");
        return -1;
    }

    tbData.game = &game;
    tbData.stop = stop;
    tbData.mutexGame = &mutexGame;

    // Opening thread responsible for reading BOT input
    if (pthread_create(&threadBotID, NULL, &threadReadBot, &tbData)){
        printf("Error creating threadPlayers\n");
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

/**
 * End the game, kicking players and notifying bots.
 *
 * @param game Pointer to the game structure.
 */
void endGame(GAME *game) {
    for(int i = 0 ; i < 5; i++) {
        kickPlayer(game, game->players[0], 1);  // Kicking Players
    }
    for(int i = 0 ; i < 5 ; i++) {
        kickPlayer(game, game->nonPlayers[0], 0);   // Kicking non-Players
    }
    for(int i = 0 ; i < game->nBots ; i++) {    // Kicking Bots
        closeBot(game);
    }

    union sigval val;
    val.sival_int = 4;        

    sigqueue(getpid(), SIGUSR2, val);   //Seding Signal to "myself"
}

/**
 * Thread function for reading input from the bot and updating the game state accordingly.
 *
 * @param data Pointer to the TBDATA structure containing necessary data.
 * @return NULL
 */
void *threadReadBot(void *data)
{
    TBDATA *tbData = (TBDATA *)data;
    char buffer[BUFFER_SIZE];
    char col[20], lin[20], duration[20];
    int bytes_read, i = 0;

    pthread_mutex_lock(tbData->mutexGame);
    for (int i = 0; i < 50; i++)
    {
        tbData->game->rocks[i].skin = 'U';
    }
    pthread_mutex_unlock(tbData->mutexGame);

    while (stop == 0)
    {
        bytes_read = read(tbData->game->pipeBot[0], buffer, 256); // Reading BOT Stdout
        if (bytes_read < 0 && stop == 0)
        {
            printf("Error reading BOT stdout\n");
        }
        else
        {

            buffer[bytes_read] = '\0'; // Null-terminate the received data
            sscanf(buffer, "%s %s %s", col, lin, duration);
            int colN = atoi(col);
            int linN = atoi(lin);
            int durationN = atoi(duration);
            pthread_mutex_lock(tbData->mutexGame);

            placeRock(colN, linN, durationN, tbData->game); // Updating Map with Rock
            pthread_mutex_unlock(tbData->mutexGame);

            printf("Received: %s %s %s\n", lin, col, duration);
        }
    }
    pthread_exit(NULL);
}

/**
 * Place a rock in the game at the specified position if the space is unoccupied.
 *
 * @param col      The column index for the rock's position.
 * @param lin      The line index for the rock's position.
 * @param duration The duration of the rock's presence in the game.
 * @param game     Pointer to the GAME structure representing the game state.
 */
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
        printf("Space for Rock was occupied [%c] %d %d\n", game->map[lin][col], lin, col);
    }
}

/**
 * Thread function responsible for managing the game clock and handling time-related events.
 *
 * @param data Pointer to CLKDATA structure containing game data and synchronization objects.
 */
void *threadClock(void *data)
{
    CLKDATA *clkData = (CLKDATA *)data;

    int size = 0;

    while (stop == 0)
    {
        sleep(1);
        pthread_mutex_lock(clkData->mutexGame);
        if (clkData->game->timeleft > 0) // Game is still ongoing
        {
            clkData->game->timeleft--;

            for (int i = 0; i < 50; i++)    // Updating Rocks
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

            for (int i = 0; i < clkData->game->nObs; i++) // Updating Obstacles
            {
                movePlayer(clkData->game, NULL, &clkData->game->obstacle[i]);
                sendMap(clkData->game);
            }
        }

        if (clkData->game->timeleft == 0 && clkData->game->level != 0) // Game Ended
        {
            printf("\nGame Ended");
            clkData->game->gameover = 1;
            sendMap(clkData->game);
            sleep(5);
            endGame(clkData->game);
        }
        else if (clkData->game->timeleft == 0 && clkData->game->level == 0 && clkData->game->nPlayers >= clkData->game->minNplayers|| clkData->game->start == 1)    // Time to Enroll has passed
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
    pthread_exit(NULL);
}

/**
 * Sends the current game state to all players and non-players through their respective FIFOs.
 *
 * @param game Pointer to the GAME structure representing the current game state.
 */
void sendMap(GAME *game) {
    int size = 0;
    char pipeNameGamePlayer[30];

    for (int i = 0; i < game->nPlayers; i++)
    {
        sprintf(pipeNameGamePlayer, FIFO_GAMEUI, game->players[i].pid);

        int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
        if (fdWrGamePlayer == -1)
        {
            printf("Erro no open fifo Player: %s\n", game->players[i].name);
            kickPlayer(game,game->players[i],1);
        }

        size = write(fdWrGamePlayer, game, sizeof(GAME));
        if (size == -1) {
            printf("Error sending Game");
        }
    }

    for (int i = 0; i < game->nNonPlayers; i++)
    {
        sprintf(pipeNameGamePlayer, FIFO_GAMEUI, game->nonPlayers[i].pid);

        int fdWrGamePlayer = open(pipeNameGamePlayer, O_RDWR);
        if (fdWrGamePlayer == -1)
        {
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeNameGamePlayer, errno);
            kickPlayer(game,game->nonPlayers[i],0);
        }

        size = write(fdWrGamePlayer, game, sizeof(GAME));
        if (size == -1) {
            printf("Error sending Game");
        }
    }
}

/**
 * Thread responsible for accepting players into the game.
 * It reads player information from a FIFO, processes the request,
 * and responds to the players.
 *
 * @param data The data structure containing information needed for the thread.
 * @see ACPDATA
 */
void *threadACP(void *data)
{ // thread to accept players
    int flag = 0, i = 0, n, cont = 0;
    ACPDATA *acpData = (ACPDATA *)data;


    int fdRdACP = open(FIFO_ENGINE_ACP, O_RDWR);
    if (fdRdACP == -1)
    {
        printf("Error opening FIFO for accepting players\n");
        union sigval val;
        val.sival_int = 4;        
        sigqueue(getpid(), SIGUSR2, val);   //Seding Signal to "myself"
    }

    while (stop == 0)
    {
        PLAYER player;
        flag = 0;
        char pipeName[50];

        int size = read(fdRdACP, &player, sizeof(PLAYER)); // Player trying to enter the game
        if (size == -1) {
            if(stop==0)
			    printf("Error reading Message");
        }

        if(stop==1){
            break;
        }

        pthread_mutex_lock(acpData->mutexGame);
        
        if (acpData->game->nPlayers < 5 && acpData->game->level == 0 && acpData->game->start == 0 && acpData->timeEnrolment > 0) // Still able to play
        {
            for (i = 0; i < acpData->game->nPlayers; i++) // Checking for repeated names
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
                switch (acpData->game->nPlayers) //Assigning skins
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
        else // Player will only be able to watch
        {
            player.accepted = 2;
            acpData->game->nonPlayers[acpData->game->nNonPlayers] = player;
            acpData->game->nNonPlayers++;

            printf("\nThe number of players for this game has been reached or time for entry as ended\n");
        }

        pthread_mutex_unlock(acpData->mutexGame);

    
        sprintf(pipeName, FIFO_GAMEUI, player.pid);
        
        //Opening Player's PIPE
        int fdWrInitGameui = open(pipeName, O_WRONLY);
        if (fdWrInitGameui == -1)
        {
            fprintf(stderr, "\nError opening %s for writing: %d\n", pipeName, errno);
        }


        size = write(fdWrInitGameui, &player, sizeof(player));  // Writing the response to the Player
        if (size == -1) {
            printf("Error sending Game");
            kickPlayer(acpData->game,player,player.accepted);        
        }
    }
    
    unlink(FIFO_ENGINE_ACP);
    close(fdRdACP);
    pthread_exit(NULL);
}

/**
 * Sets environment variables needed for the game initialization.
 * These variables include enrollment time, the number of players, 
 * game duration, and the decrement value for obstacles.
 */
void setEnvVars()
{
    setenv("ENROLLMENT", "60", 1); // Create the variable, overwriting if exist (1)
    setenv("NPLAYERS", "2", 1);
    setenv("DURATION", "60", 1);
    setenv("DECREMENT", "5", 1);
}

/**
 * Retrieves environment variables related to game configuration and initializes
 * corresponding fields in the provided structures (GAME and ACPDATA).
 * The method reads variables such as ENROLLMENT time, the minimum number of players,
 * game duration, and the decrement value for obstacles.
 *
 * @param game    The GAME structure to store game-related environment variables.
 * @param acpData The ACPDATA structure to store enrollment-related environment variables.
 */
void getEnvVars(GAME *game, ACPDATA *acpData)
{
    char *p = NULL;

    p = getenv("ENROLLMENT");   // Read variable with a specific name
    if (p != NULL)
    {
        printf("\nVariable: ENROLLMENT = %s", p);
        acpData->timeEnrolment = (int)*p;
        game->timeleft = acpData->timeEnrolment;
    }

    p = getenv("NPLAYERS");   // Read variable with a specific name
    if (p != NULL)
    {
        printf("\nVariable: NPLAYERS = %s", p);
        game->minNplayers = (int)*p;
    }

    p = getenv("DURATION");   // Read variable with a specific name
    if (p != NULL)
    {
        printf("\nVariable: DURATION = %s", p);
        game->time = (int)*p;
    }

    p = getenv("DECREMENT");   // Read variable with a specific name
    if (p != NULL)
    {
        printf("\nVariable: DECREMENT = %s\n", p);
        game->timeDec = atoi(p);
    }
}

/**
 * Creates a pipe for communication between the game and the bot. The pipe is
 * used to redirect the bot's standard output (stdout) to the game for processing.
 *
 * @param pipeBot An array to store file descriptors of the pipe for bot communication.
 * @param game    The GAME structure where the pipe information will be stored.
 */
void createPipe(int *pipeBot, GAME *game)
{
    pipe(pipeBot);
    game->pipeBot = pipeBot;
}

/**
 * Launches bots for the game based on the current game level. Each bot is created
 * as a separate process with its own process ID (PID), and its standard output (stdout)
 * is redirected to the game for processing through a pipe.
 *
 * @param game The GAME structure where information about the game and bots will be stored.
 */
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

        sprintf(coordS, "%d", coord);
        sprintf(durationS, "%d", duration);

        if (pidBot < 0)
        {
            perror("Error creating Bot (Failed Fork())\n");
            continue;
        }
        if (pidBot == 0) // Inside cloned process
        {
            if (dup2(game->pipeBot[1], STDOUT_FILENO) == -1)    // Overwritting the standard output of the cloned process with the anonymous pipe write end
            {
                printf("dup2\n");
                exit(EXIT_FAILURE);
            }
            close(game->pipeBot[1]);
            close(game->pipeBot[0]);

            int res = execl("bot/bot.exe", "bot", coordS, durationS, NULL); // Executing Bot program on the cloned process
            if (res != 0)
            {
                printf("Failed Execl\n");
                exit(EXIT_FAILURE);
            }
        }
        else   // Back on the Original process
        {
            bot.pid = pidBot;
            bot.duration = coord;
            bot.interval = duration;
            game->bots[game->nBots++] = bot;
            coord = coord - 5;
            duration = duration - 5;

        }
    }
}

/**
 * Closes the specified bots in the game.
 *
 * This function sends a SIGINT signal to each bot in the game to initiate the
 * closing process. It waits for each bot to terminate and updates the game's
 * information accordingly.
 *
 * @param game Pointer to the GAME structure.
 */
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
    }
}

/**
 * Initializes a new game, setting initial values for various parameters.
 *
 * This function resets the game state, preparing it for a new session. It sets
 * the game level, the number of players, bots, non-players, obstacles, and rocks
 * to zero. Additionally, it sets the game start flag to zero.
 *
 * @param game Pointer to the GAME structure.
 */
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

/**
 * Thread function to handle keyboard inputs for game control and player interaction.
 *
 * This function runs in a separate thread to read and process keyboard inputs.
 * It supports various commands to manage the game state, including kicking players,
 * starting or exiting the game, viewing players and bots, and more.
 *
 * @param data Pointer to the KBDATA structure containing thread-specific data.
 */
void *threadKBEngine(void *data)
{ 
    KBDATA *kbData = (KBDATA *)data;
    char cmd[200], str1[30], str2[30];

    while (stop == 0)
    {
        int flag = 0;
        printf("\nCommand: ");
        fflush(stdout);
        fgets(cmd, sizeof(cmd), stdin);
        printf("\n");

        if (sscanf(cmd, "%s %s", str1, str2) == 2 && strcmp(str1, "kick") == 0) // Command "kick"
        {
            pthread_mutex_lock(kbData->mutexGame);
            for (int i = 0; i < 5; i++)
            {                
                if (strcmp(kbData->game->players[i].name, str2) == 0)
                {
                    kickPlayer(kbData->game, kbData->game->players[i], 1);
                    
                    flag = 1;
                    break;
                }
            }
            for (int i = 0; i < 5; i++)
            {                
                if (strcmp(kbData->game->nonPlayers[i].name, str2) == 0)
                {
                    kickPlayer(kbData->game, kbData->game->nonPlayers[i], 0);
                    
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
                if (strcmp(cmd, "begin") == 0)  // Command "Begin"
                { 
                    pthread_mutex_lock(kbData->mutexGame);
                    kbData->game->start = 1;
                    passLevel(kbData->game);
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "exit") == 0)
                {
                    pthread_mutex_lock(kbData->mutexGame);
                     union sigval val;
                    val.sival_int = 4;  

                    for(int i = 0 ; i < 5; i++) {
                        kickPlayer(kbData->game, kbData->game->players[0], 1);
                    }
                    for(int i = 0 ; i < 5 ; i++) {
                        kickPlayer(kbData->game, kbData->game->nonPlayers[0], 0);
                    }      

                    sigqueue(getpid(), SIGUSR2, val);
                    
                    pthread_mutex_unlock(kbData->mutexGame);
                }
            } else {
                if (strcmp(cmd, "users") == 0)  // Command "Users"
                {
                    pthread_mutex_lock(kbData->mutexGame);
                    printf("\nPlayers:\n");

                    for (int i = 0; i < kbData->game->nPlayers; i++)
                    {
                        printf("\tPlayer %s\n", kbData->game->players[i].name);
                    }

                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else if (strcmp(cmd, "bots") == 0)  // Command "Bots"
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
                else if (strcmp(cmd, "bmov") == 0) // Command "BMOV"
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
                else if (strcmp(cmd, "rbm") == 0) // Command "RBM"
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
                else if (strcmp(cmd, "exit") == 0)  // Command "EXIT"
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
                    val.sival_int = 4;        

                    sigqueue(getpid(), SIGUSR2, val);
                    
                    pthread_mutex_unlock(kbData->mutexGame);
                }
                else
                {   
                    if(stop==0)
                        printf("INVALID\n");
                }
            }
            
        }
    }
    pthread_exit(NULL);
}

/**
 * Remove a dynamic obstacle from the game.
 *
 * This function removes the first dynamic obstacle in the game, updates the game map,
 * and sends the updated map to all players.
 *
 * @param game Pointer to the GAME structure representing the game state.
 */
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

/**
 * Insert a dynamic obstacle into the game.
 *
 * This function generates random coordinates (x, y) and checks if the position in the
 * game map is empty. If it is, the function adds a dynamic obstacle to the game,
 * updates the game map, and increments the number of dynamic obstacles.
 *
 * @param game Pointer to the GAME structure representing the game state.
 */
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

/**
 * Read the game map from a file based on the specified level.
 *
 * This function reads the content of a file representing the game map for a given
 * level and updates the game map accordingly.
 *
 * @param level The level for which to read the map.
 * @param game Pointer to the GAME structure representing the game state.
 */
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
        while ((c = fgetc(file)) != EOF && c != '\n');
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

/**
 * Move a player or dynamic obstacle on the game map.
 *
 * This function moves a player or dynamic obstacle on the game map based on the
 * specified direction. It updates the game map and the position of the player or
 * obstacle accordingly. If a player reaches the top of the map, it increments the
 * player's score and passes the level.
 *
 * @param game Pointer to the GAME structure representing the game state.
 * @param player Pointer to the PLAYER structure representing the player (or NULL for a dynamic obstacle).
 * @param obstacle Pointer to the DINAMICOBS structure representing the dynamic obstacle (or NULL for a player).
 */
void movePlayer(GAME *game, PLAYER *player, DINAMICOBS *obstacle)
{ // meter esta func bem
    int flagWin = 0;
    int move;
    char skin;
    int lin;
    int col;
    if (player == NULL) // dynamic obstacle
    { 
        srand(time(NULL));
        move = rand() % 4 + 1;
        skin = obstacle->skin;
        lin = obstacle->position[0];
        col = obstacle->position[1];
    }
    else   // Player
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

    if (player == NULL) // dynamic obstacle
    { 
        
        obstacle->position[0] = lin;
        obstacle->position[1] = col;
    }
    else if(player != NULL && flagWin == 0) // Player
    {
        player->position[0] = lin;
        player->position[1] = col;
    }
    flagWin = 0;

}

/**
 * Thread function for handling player moves and messages.
 *
 * This function is responsible for handling player moves and messages. It reads
 * player moves from a FIFO, processes the moves, and updates the game state accordingly.
 * It also handles messages sent by players and sends responses to the respective players.
 *
 * @param data Pointer to the PLAYERSDATA structure containing necessary data for the thread.
 */
void *threadPlayers(void *data)
{
    PLAYERSDATA *plData = (PLAYERSDATA *)data;
    int size = 0;
    PLAYER player;
    MESSAGE msg;
    int flag = 1;

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
        size = read(fdRdPlayerMoves, &player, sizeof(PLAYER));
        if (size == -1 && stop == 0) {
                printf("Error reading Player Moves");
        }	

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

        if (player.move == -2) // Player wants to send a Message
        { 
            pthread_mutex_lock(plData->mutexGame);
            for (int i = 0; i < plData->game->nPlayers + 1; i++)
            {
                if (strcmp(player.personNameMessage, plData->game->players[i].name) == 0)
                {
                    sprintf(msg.pipeName, FIFO_PRIVATE_MSG, plData->game->players[i].pid);

                   
                    char pipeNamePIDPlayer[30];
                    sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, player.pid);

                    strcpy(plData->game->players[indice].personNameMessage, player.name);
                    strcpy(plData->game->players[indice].message, player.message);

                    int fdWrPIDPlayer = open(pipeNamePIDPlayer, O_WRONLY); // Opening Player's pipe
                    if (fdWrPIDPlayer == -1)
                    {
                        printf("Error openning pid message fifo\n");
                    }

                    size = write(fdWrPIDPlayer, &msg, sizeof(MESSAGE));
                    if (size == -1) {
                        printf("Error writing information to Player");
                    }	
                    close(fdWrPIDPlayer);

                    flag = 0;
                    break;
                }
                flag = 1;
            }

            if (flag == 1) // No player with the received name was found
            {
                char pipeNamePIDPlayer[30];
                sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, player.pid);

                int fdWrPIDPlayer = open(pipeNamePIDPlayer, O_RDWR);
                if (fdWrPIDPlayer == -1)
                {
                    printf("Error openning pid message fifo\n");
                }

                strcpy(msg.pipeName, "error");

                size = write(fdWrPIDPlayer, &msg, sizeof(MESSAGE));
                if (size == -1) {
                    printf("Error writing information to Player");
                }	
                close(fdWrPIDPlayer);
            }
            pthread_mutex_unlock(plData->mutexGame);
        }
        else if (player.move == -1) // Player sent a command
        { 
            if (strcmp(player.command, "players") == 0) // Command "Players"
            {
                pthread_mutex_lock(plData->mutexGame);

                for (int i = 0; i < plData->game->nPlayers; i++)
                {
                    strcpy(player.players[i].name, plData->game->players[i].name);
                }

                pthread_mutex_unlock(plData->mutexGame);
                
            }
            
        }
        else if (player.move == -3) // Command "Exit"
        {
            pthread_mutex_lock(plData->mutexGame);
            kickPlayer(plData->game, player, 1);
            pthread_mutex_unlock(plData->mutexGame);
        }
        else // Regular Move
        { 
            for (int i = 0; i < plData->game->nPlayers; i++)
            {
                if (strcmp(plData->game->players[i].name, player.name) == 0)
                {
                    movePlayer(plData->game, &plData->game->players[i], NULL);
                    break;
                }
            }
           
            pthread_mutex_lock(plData->mutexGame);
            sendMap(plData->game);
            pthread_mutex_unlock(plData->mutexGame);
        }
    }

    unlink(FIFO_ENGINE_GAME);
    close(fdRdPlayerMoves);
    pthread_exit(NULL);
}

/**
 * Place players on the game map.
 *
 * This function is responsible for placing players on the game map according to their
 * initial positions and skins.
 *
 * @param game Pointer to the GAME structure representing the game state.
 */
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

/**
 * Move the game to the next level.
 *
 * This function is responsible for advancing the game to the next level. It updates
 * the game state, including the map, player positions, and other parameters for the
 * new level.
 *
 * @param game Pointer to the GAME structure representing the game state.
 */
void passLevel(GAME *game)
{
    int score = 0;
    if(game->level == 3) { // Game ended
        game->win = 1;
        for(int i = 0 ; i < game->nPlayers ; i++) {
            if(score < game->players[i].score) {
                score = game->players[i].score;
                strcpy(game->winPlayer, game->players[i].name);
            }
        }
        sendMap(game);
        sleep(5);
        endGame(game);
        return;
    }

    game->start = 1;
    printf("\nGame level: %d\n", game->level);
    if (game->level == 0) { // First Level
        game->time = 105; // time of level 1
    }
    game->level += 1;
    readFileMap(game->level, game); // Read new Map
    placePlayers(game); // Updates players
    closeBot(game); // Close all the previous level Bots
    launchBot(game);    // Lauches new level Bots

    game->time = game->time - game->timeDec;
    game->timeleft = game->time;

    printf("Time of the level: %d\n", game->time);
    printf("Timedec: %d\n", game->timeDec);
    printf("TimeLeft: %d\n", game->timeleft);
}

/**
 * Kick a player from the game.
 *
 * This function is responsible for kicking a player from the game based on the
 * provided PLAYER structure and a flag indicating whether the kick is accepted or not.
 * It sends a signal to the player and removes the player from the game.
 *
 * @param game Pointer to the GAME structure representing the game state.
 * @param player The PLAYER structure representing the player to be kicked.
 * @param accepted Flag indicating whether the player is accepted in the game (1) or not (0).
 */
void kickPlayer(GAME *game, PLAYER player, int accepted) {
    union sigval val;
    val.sival_int = 3;

    // send signal to player and remove him from the game
    if(accepted == 0) {
        for (int i = 0; i < game->nNonPlayers; i++)
        {
            if(game->nonPlayers[i].pid == player.pid){
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

/**
 * Signal handler for the game engine.
 *
 * This function handles signals received by the game engine. It currently handles
 * SIGINT and SIGUSR2 signals. For SIGINT, the function can be extended with appropriate
 * actions when the game engine receives an interrupt signal. For SIGUSR2, the 'stop' flag
 * is set to 1, indicating the termination of the game.
 *
 * @param signum The signal number.
 */
void handlerSignalEngine(int signum) {
    if (signum == SIGINT) {

    } else if (signum == SIGUSR2) {
		stop = 1;
    }
}