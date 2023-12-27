#include "gameui.h"

int stop;

/**
 * The main function of the gameUI program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return Returns 0 upon successful execution.
 */
int main(int argc, char *argv[]) {
    int lin, col, color;
	char pipeName[50];
	char pipeNameRecEngine[30];
    WINDOW *wGame, *wComands, *wGeneral, *wInfo;
    PLAYER player;
	PLAYDATA playData;
	RECGAMEDATA recGameData;
	RECMSGDATA recMSGData;
	pthread_t threadPlayID, threadRecGameID, threadRecMSGID;
    stop = 0;

	// Struct for SIGWINCH, responsible for handling terminal resizing
	struct sigaction sa_winch;
    sa_winch.sa_handler = handlerSignalGameUI;
    sigemptyset(&sa_winch.sa_mask);
    sa_winch.sa_flags = SA_SIGINFO;

	// Struct for SIGINT, responsible for handling CTRL+C signal
	struct sigaction sa_int;
    sa_int.sa_handler = handlerSignalGameUI;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_SIGINFO;

	// Struct for SIGUSR1, responsible initiating the orderly end of the process
	struct sigaction sa_usr1;
    sa_usr1.sa_handler = handlerSignalGameUI;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_SIGINFO;


    if(argc != 2) {
        printf("\n[ERROR] Invalid number of arguments -> Sintax: <./gameui NAME>\n");
        exit(1);
    }

    strcpy(player.name, argv[1]);

	// Opening PIPE responsible for entering the game
	int fdFIFO_ENGINE_ACP = open(FIFO_ENGINE_ACP, O_WRONLY);  
	if(fdFIFO_ENGINE_ACP == -1) {
		printf("Engine is not running yet\n");     
		return -1;
	}

	// signal(SIGWINCH, resizeHandler);
    // if (sigaction(SIGWINCH, &sa_winch, NULL) == -1) {
    //     printf("Unable to register SIGWINCH signal handler");
    // }

	if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        printf("Unable to register SIGUSR1 signal handler\n");
    }
	if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        printf("Unable to register SIGINT signal handler\n");
    }

	// Creating PIPE responsible for receiving Data from Engine
	sprintf(pipeNameRecEngine, FIFO_GAMEUI, getpid());
	if(mkfifo(pipeNameRecEngine, 0666) == -1) {
		printf("Error creating pid message fifo\n");
	}

	int fdFIFO_GAMEUI = open(pipeNameRecEngine, O_RDWR);
    if(fdFIFO_GAMEUI == -1) {
        printf("Error openning engine game fifo\n");
		
    }

	player.pid = getpid();
	
	// Sending information to Engine, in order to know if the player is accepted into the game
	int size = write(fdFIFO_ENGINE_ACP, &player, sizeof(PLAYER));
	if (size == -1) {
		printf("Error writing to Engine\n");
		return -1;
	}

	// Receive confirmation about the acceptance in the game
	size = read(fdFIFO_GAMEUI, &player, sizeof(player));
	if (size == -1) {
		printf("Error reading from fdFIFO_GAMEUI\n");
		unlink(pipeNameRecEngine);
		close(fdFIFO_GAMEUI);
		return -1;
	}

	// Another Player has this Player's name, therefore, he will not be able to play
	if(player.accepted == 0) {	
		printf("There are already someone with your name or the time to enter the game ended\n"); 
		unlink(pipeNameRecEngine);
		close(fdFIFO_GAMEUI);
		return -1;
	} 

	initscr();       // Initialize ncurses and set up the terminal
    erase();         // Clear the screen and position the cursor at the top-left corner 
    noecho();        // Turn off echoing to prevent displaying pressed keys on the screen
    cbreak();        // Disable line buffering, each key is read immediately (Enter is not needed in order for the process to recognize the keys)
    refresh();       

	// Initialize color pairs for later us
	init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
	refresh(); 

 	// Create windows with specified dimensions and positions
	wGame= newwin(18, 42, 1, 1);  
	wInfo = newwin(7, 42, 19, 1); 
	wGeneral = newwin(6, 42, 26, 1);   
	wComands =  newwin(5, 42, 32, 1);  
	
	// Draw boxes around each window
	box(wGame,0,0);
	box(wComands,0,0);
	box(wGeneral,0,0);
	box(wInfo,0,0);
	
	noecho();   // Desliga o echo porque a seguir vai ler telcas de direção não se pretende "ecoar" essa tecla no ecrã
	cbreak();   // desliga a necessidade de enter. cada tecla é lida imediatamente
	
	// Set color pair for the entire screen
	attron(COLOR_PAIR(5));

	// Set foreground and background colors for the game window
    wattrset(wGame, COLOR_PAIR(4));   // Set foreground/background colors for characters
    wbkgd(wGame, COLOR_PAIR(4));       // Set background color for empty spaces
    scrollok(wGame, TRUE);             // Enable scrolling in the game window
    keypad(wGame, TRUE);               // Enable processing of special keys in the game window

 	// Refresh all windows
	wrefresh(wGame);
	wrefresh(wComands);
	wrefresh(wGeneral);
	wrefresh(wInfo);

	int fdFIFO_ENGINE_GAME = open(FIFO_ENGINE_GAME, O_WRONLY);	// Opening PIPE to send Player's moves
	
	if(player.accepted == 1) {	// Player was successfully accepted into the game
		GAME game;

		playData.player = &player;
		playData.stop = stop;
		playData.window = wComands;
		playData.fdFIFO_ENGINE_GAME = fdFIFO_ENGINE_GAME;

		readMap(fdFIFO_GAMEUI,wGame,wInfo);

		// Opening Thread responsible for Player's interaction with the process
		if(pthread_create(&threadPlayID, NULL, &threadPlay, &playData)) {
			printf("Error creating threadPlay\n");
			playData.player->move = -3;
			size = write (playData.fdFIFO_ENGINE_GAME, playData.player, sizeof(PLAYER));
		}

		recMSGData.window = wGeneral;
		recMSGData.stop = stop;

		// Opening Thread responsible for receiving private Messages from other Players
		if(pthread_create(&threadRecMSGID, NULL, &threadRecMessages, &recMSGData)) {
			printf("Error creating threadRecGame\n");
			playData.player->move = -3;
			size = write (playData.fdFIFO_ENGINE_GAME, playData.player, sizeof(PLAYER));
		}
	} else {	// Player is only watching the game
		readMap(fdFIFO_GAMEUI,wGame,wInfo);
	}
	
	recGameData.stop = stop;
	recGameData.window = wGame;
	recGameData.wInfo = wInfo;
	recGameData.fdFIFO_GAMEUI = fdFIFO_GAMEUI;

	// Opening Thread responsible for receiving STRUCT GAME from Engine
	if(pthread_create(&threadRecGameID, NULL, &threadRecGame, &recGameData)) {
		printf("Error creating threadRecGame\n");
		player.move = -3;
		size = write (fdFIFO_ENGINE_GAME, &player, sizeof(PLAYER));
	}

	while (1) {
		if (stop == 1) {
			if (player.accepted == 1) { // If the player is accepted into the game
				pthread_kill(threadPlayID, SIGUSR1);    // Send SIGUSR1 signal to threadPlayID
				pthread_kill(threadRecMSGID, SIGUSR1);  // Send SIGUSR1 signal to threadRecMSGID
			}
			pthread_kill(threadRecGameID, SIGUSR1);     // Send SIGUSR1 signal to threadRecGameID
			break;  
		}
	}

	if (player.accepted == 1) {
		pthread_join(threadPlayID, NULL);       // Wait for threadPlayID to finish
		pthread_join(threadRecMSGID, NULL);    // Wait for threadRecMSGID to finish
	}

	pthread_join(threadRecGameID, NULL);    // Wait for threadRecGameID to finish

	unlink(pipeNameRecEngine);    // Remove the named pipe
	close(fdFIFO_GAMEUI);         // Close the file descriptor

	// Clear and erase the contents of each window
	werase(wGame);
	werase(wInfo);
	werase(wGeneral);
	werase(wComands);

	// Clear the window buffers
	wclear(wGame);
	wclear(wInfo);
	wclear(wGeneral);
	wclear(wComands);

	// Delete each window
	delwin(wGame);
	delwin(wInfo);
	delwin(wGeneral);
	delwin(wComands);
   	endwin();

   	return 0;
}

/**
 * Reads a command from the user interface, processes it, and updates the player structure accordingly.
 *
 * @param player The player structure to be updated based on the command.
 * @param wComands The window where the command is displayed and input is received.
 * @return An integer indicating the type of command processed: 1 for "players", 2 for "msg", -1 for "exit", or 0 for an invalid command.
 */
int keyboardCmdGameUI(PLAYER *player, WINDOW *wComands) {
    char str1[10], str2[30], str3[100];
    char cmd[200], msg[200];

	werase(wComands);
	box(wComands,0,0);

	wrefresh(wComands);
	mvwprintw(wComands,1,1, "Command: ");

	fflush(stdout);
	wscanw(wComands," %200[^\n]", cmd);
	
	if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {	// Message to another Player

		strcpy(player->command, str1);
		strcpy(player->personNameMessage, str2);
		strcpy(player->message, str3);

		return 2;
	} else {
		if(strcmp(cmd, "players") == 0) {	// Command "Players"
			strcpy(player->command, cmd);
			return 1;
		}else if(strcmp(cmd, "exit") == 0) {	// Command "exit"
			strcpy(player->command, cmd);
			return -1;
		} else {								// Invalid Command
			return 0;
		}
	}
}

/**
 * Thread function for handling player input (Game moves and Commands) and communication with the game engine.
 *
 * @param data A pointer to a structure containing thread-specific data.
 * @return None.
 */
void *threadPlay(void *data) { 
	PLAYDATA *plData = (PLAYDATA *) data;
	const char *p;
	int ch;
	int ret;
	int size;

	p = NULL;

	char pipeNamePIDPlayer[30];

	// Creating PIPE for receiving information about destinatary's PID from ENGINE
	sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, getpid());
	if(mkfifo(pipeNamePIDPlayer, 0666) == -1) {
		printf("\nError creating pid message fifo\n");
	}

	keypad(plData->window, TRUE);	// Enable keypad input in the specified game Window, to allow the program to capture special keys (such as function keys or arrow keys)
	
    while (stop == 0) {
		wmove(plData->window, 1, 1); // Moves cursor to window Game
		size = 0;
		ret = 0;
       	ch = wgetch(plData->window); 
       	switch (ch) {
			case KEY_UP:
				p = "up";
				plData->player->move = 1;
				break;
			case KEY_DOWN:
				p = "down";
				plData->player->move = 3;
				break;
			case KEY_LEFT:
				p = "left";
				plData->player->move = 0;
				break;
			case KEY_RIGHT:
				p = "right";
				plData->player->move = 2;
				break;
			case ' ':
				echo();          // Turning echo on to display pressed keys on screen
				nocbreak();      // Enter will be needed to input characters

				ret = keyboardCmdGameUI(plData->player, plData->window);

				if(ret == -1) {					// Command "Exit"
					plData->player->move = -3; 
				} else if(ret == 1) {			// Command "Players"
					plData->player->move = -1;
				} else if(ret == 2) {			// Private Message
					plData->player->move = -2;
					
					size = write (plData->fdFIFO_ENGINE_GAME, plData->player, sizeof(PLAYER));	//Sending the destinatary of the message to ENGINE
					if (size == -1) {
							printf("Error information to Engine");
					}	
					MESSAGE msg;
					refresh();

					
					int fdRdEngPIDPlayer = open(pipeNamePIDPlayer, O_RDONLY);
					if(fdRdEngPIDPlayer == -1) {
						printf("Error openning pid message fifo\n"); 
					}
					
					size = read(fdRdEngPIDPlayer, &msg, sizeof(msg));	// Reading the destinatary's PID
					close(fdRdEngPIDPlayer);

					if (size > 0){
						refresh();

						if(strcmp(msg.pipeName, "error") == 0)	 // ENGINE didn't found anyone by the name that we sent
							break;
						
						strcpy(msg.msg, plData->player->message);
						strcpy(msg.namePlayerSentMessage, plData->player->name);

						int fdWrPlayer = open(msg.pipeName, O_RDWR);	// Opening the destinary's PIPE that receives messages
						if(fdWrPlayer == -1) {
							printf("Error openning private message fifo\n"); 
						}

						size = write (fdWrPlayer, &msg, sizeof(MESSAGE));
						if (size == -1) {
							printf("Error sending Message");
						}
						close(fdWrPlayer);
					}		
				}

				noecho();        
				cbreak();       
				break;
    	}

		if(stop!=1){
			if(plData->player->move != -2) {
				//send player to engine ENGINE FIFO GAME
				size = write (plData->fdFIFO_ENGINE_GAME, plData->player, sizeof(PLAYER));
				if (size == -1) {
					printf("Error sending move to Player");
				}
			}
		}
		
      	if (p != NULL) {
         	p = NULL;
         	wrefresh(plData->window); 
      	}
   	}
	
	close(plData->fdFIFO_ENGINE_GAME);
	unlink(pipeNamePIDPlayer);
	pthread_exit(NULL);
}

/**
 * Reads the game map from a FIFO and updates the windows with the new map.
 *
 * @param fdFIFO_GAMEUI The file descriptor of the FIFO for communication with the game engine.
 * @param wGame The window where the game map is displayed.
 * @param wInfo The window where additional game information is displayed.
 * @return None.
 */
void readMap(int fdFIFO_GAMEUI, WINDOW * wGame, WINDOW* wInfo){
	GAME game;
	int size = 0;

	size = read (fdFIFO_GAMEUI, &game, sizeof(GAME));
	if (size == -1 & stop ==0 ) {
		printf("Error reading Map");
	}
	printmap(game, wGame, wInfo);

	wrefresh(wGame);
	wrefresh(wInfo);
}

/**
 * Thread function for continuously receiving and updating the game map.
 *
 * @param data A pointer to a structure containing thread-specific data.
 * @return None.
 */
void *threadRecGame(void *data) {
	RECGAMEDATA *recGData = (RECGAMEDATA *) data;
	int size = 0;
	while(stop == 0) {
		wrefresh(recGData->window);
		box(recGData->window,0,0);

		readMap(recGData->fdFIFO_GAMEUI,recGData->window,recGData->wInfo);
	}
	pthread_exit(NULL);
}

/**
 * Thread function for continuously receiving and displaying private messages.
 *
 * @param data A pointer to a structure containing thread-specific data.
 * @return None.
 */
void *threadRecMessages(void *data) {
	RECMSGDATA *recMSGData = (RECMSGDATA *) data; 
	MESSAGE msg;
	char pipeNamePrivMSG[30];
	int size = 0;

	// Creating FIFO to receive private Messages
	sprintf(pipeNamePrivMSG, FIFO_PRIVATE_MSG, getpid());
	if(mkfifo(pipeNamePrivMSG, 0666) == -1) {
		printf("Error creating private message fifo\n");
	}

	int fdFIFO_PRIVATE_MSG = open(pipeNamePrivMSG, O_RDWR);
	if(fdFIFO_PRIVATE_MSG == -1) {
		printf("Error openning private message fifo\n"); 
	}

	while(stop == 0) {
		//read from pipe PRIVATE MSG PID
		size = read (fdFIFO_PRIVATE_MSG, &msg, sizeof(MESSAGE));
		if (size == -1) {
			werase(recMSGData->window);
			box(recMSGData->window,0,0);

			wrefresh(recMSGData->window);
			mvwprintw(recMSGData->window,1,1, "Error reading Private Message");
			wrefresh(recMSGData->window);
			refresh();
		}else{
			//show player the msg from the other player
			werase(recMSGData->window);
			box(recMSGData->window,0,0);

			wrefresh(recMSGData->window);
			mvwprintw(recMSGData->window,1,1, "Player %s sent you: %s", msg.namePlayerSentMessage, msg.msg);


			wrefresh(recMSGData->window);
			refresh();
		}
	}
	unlink(pipeNamePrivMSG);
	close(fdFIFO_PRIVATE_MSG);
	pthread_exit(NULL);

}

/**
 * Signal handler function for handling specific signals in the Game UI.
 *
 * @param signum The signal number received by the handler.
 * @return None.
 */
void handlerSignalGameUI(int signum) {
	if (signum == SIGINT) {
    } else if (signum == SIGWINCH) {
		refresh();
    } else if (signum == SIGUSR1) {
		stop = 1;
    }
}

/**
 * Displays the game map and relevant information in the specified windows.
 *
 * @param game The game structure containing map and game information.
 * @param wGame The window where the game map is displayed.
 * @param wInfo The window where additional game information is displayed.
 * @return None.
 */
void printmap(GAME game, WINDOW* wGame, WINDOW * wInfo){
	int flag = 0;
	init_pair(1, COLOR_YELLOW, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_RED);

	if(game.gameover == 1) {	// Game Ended
		werase(wGame);
		box(wGame,0,0);

		wrefresh(wGame);
		for(int i= 0;i<game.nPlayers;i++){
			if(game.players[i].pid == getpid()) {
				mvwprintw(wGame,1,1, "GAME OVER");
				flag = 1;
				break;
			}		
		}
		if(flag == 0) {
			for(int i= 0;i<game.nNonPlayers;i++){
				if(game.nonPlayers[i].pid == getpid()) {
					mvwprintw(wGame,1,1, "GAME ENDED");
					flag = 0;
					break;
				}		
			}
		}
	} 
	else if(game.win == 1) {	// Someone Won
		werase(wGame);
		box(wGame,0,0);

		wrefresh(wGame);
		for(int i = 0 ; i < game.nPlayers ; i++){
			if(game.players[i].pid == getpid()) {
				if(strcmp(game.winPlayer, game.players[i].name) == 0) {
					mvwprintw(wGame,1,1, "YOU WON!");
					flag = 1;
					break;
				}
			}
		}
		for(int i= 0;i<game.nNonPlayers;i++){
			if(game.nonPlayers[i].pid == getpid()) {
				mvwprintw(wGame,1,1, "GAME ENDED");
				flag = 1;
				break;
			}		
		}
		if(flag == 0) {
			mvwprintw(wGame,1,1, "YOU LOST!");
		}
		flag = 0;
	} 
	else {	// Regular printing of the Map
		for(int i = 0 ; i < 16 ; i++) {
			for(int j=0 ; j < 40 ; j++){
				if(game.map[i][j] == 'x'){
					attron(COLOR_PAIR(2));
					mvwprintw(wGame,i+1,j+1,"X");
					attroff(COLOR_PAIR(2));
				}
				else if(game.map[i][j] == ' '){
					mvwprintw(wGame,i+1,j+1," ");
				} 
				else if(game.map[i][j] == '0') {
					mvwprintw(wGame,i+1,j+1,"0");
				}
				else if(game.map[i][j] == '1') {
					mvwprintw(wGame,i+1,j+1,"1");
				}
				else if(game.map[i][j] == '2') {
					mvwprintw(wGame,i+1,j+1,"2");
				}
				else if(game.map[i][j] == '3') {
					mvwprintw(wGame,i+1,j+1,"3");
				}
				else if(game.map[i][j] == '4') {
					mvwprintw(wGame,i+1,j+1,"4");
				}
				else if(game.map[i][j] == 'R') {
					mvwprintw(wGame,i+1,j+1,"R");
				}
				else if(game.map[i][j] == 'W') {
					mvwprintw(wGame,i+1,j+1,"W");
				}
			}
		}

		// Information About the Game
		mvwprintw(wInfo,1,1,"Moving Obstacle: %d",game.nObs);
		mvwprintw(wInfo,2,1,"Rocks: %d",game.nRocks);

		mvwprintw(wInfo, 3, 1, "Players: ");
		for(int i= 0;i<game.nPlayers;i++){
			wprintw(wInfo, "%s", game.players[i].name);

			if (i < game.nPlayers - 1) {
				wprintw(wInfo, ", ");
			}
		}

		mvwprintw(wInfo, 4, 1, "Time: %d", game.timeleft);
		for(int i= 0;i<game.nPlayers;i++){
			if(game.players[i].pid == getpid()) {
				mvwprintw(wInfo, 5, 1, "Player: %c", game.players[i].skin);
				mvwprintw(wInfo, 5, 13, "   Score: %d", game.players[i].score);
				break;
			}		
		}
	}
}