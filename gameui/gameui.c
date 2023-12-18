#include "gameui.h"



int main(int argc, char *argv[]) {
    int lin, col, color, stop = 0;
	char pipeName[50];
    WINDOW *wGame, *wComands, *wGeneral, *wInfo;
    
    PLAYER player;
	PLAYDATA playData;
	RECGAMEDATA recGameData;
	RECMSGDATA recMSGData;
	pthread_t threadPlayID, threadRecGameID, threadRecMSGID;
    
    if(argc != 2) {
        printf("\n[ERROR] Invalid number of arguments -> Sintax: <./gameui NAME>\n");
        exit(1);
    }
	
    strcpy(player.name, argv[1]);

	int fdWrInitEngine = open(FIFO_ENGINE_ACP, O_WRONLY);   //Abrir o fifo em modo de escrita bloqueante
	if(fdWrInitEngine == -1) {
		perror("Engine is not running yet\n");     
		return -1;
	}

	signal(SIGWINCH, resizeHandler);

	char pipeNameRecEngine[30];

	sprintf(pipeNameRecEngine, FIFO_GAMEUI, getpid());

	if(mkfifo(pipeNameRecEngine, 0666) == -1) {
		perror("\nError creating pid message fifo\n");
	}

	int fdRdEngine = open(pipeNameRecEngine, O_RDWR);
    if(fdRdEngine == -1) {
        perror("Error openning engine game fifo\n"); 
    }

	player.pid = getpid();
	
	//send player to engine to see if he can enter the gamemm
	int size = write (fdWrInitEngine, &player, sizeof(PLAYER));
	printf("Sent: %s com o tamanho [%d]\n", player.name, size);

    //receive response from engine to see if the player can enter the game
	size = read (fdRdEngine, &player, sizeof(player));

	if(player.accepted == 0) {	//cannot play because there are already someone with that name
		printf("There are already someone with your name or the time to enter the game ended\n"); 
		//unlink(pipeName);
		//close(fdRdEngine);
		return -1;
	} 

	initscr();  // Obrigatorio e sempre a primeira operação de ncurses
             // Inicializa o motor de cores
    erase();      // Limpa o ecrã e posiciona o cursor no canto superir esquerdo (refresh mais adiante)
    noecho();   // Desliga o echo porque a seguir vai ler telcas de direção não se pretende "ecoar" essa tecla no ecrã
    cbreak();   // desliga a necessidade de enter. cada tecla é lida imediatamente
	refresh(); 

	init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLUE);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
	refresh(); 

	wGame= newwin(18, 42, 1, 1);  
	wInfo = newwin(7, 42, 19, 1); 
	wGeneral = newwin(6, 42, 26, 1);   
	wComands =  newwin(5, 42, 32, 1);  
	

	box(wGame,0,0);
	box(wComands,0,0);
	box(wGeneral,0,0);
	box(wInfo,0,0);
	
	noecho();   // Desliga o echo porque a seguir vai ler telcas de direção não se pretende "ecoar" essa tecla no ecrã
	cbreak();   // desliga a necessidade de enter. cada tecla é lida imediatamente
	
	attron(COLOR_PAIR(5));

	//Windows
	wattrset(wGame, COLOR_PAIR(4));  // define foreground/backgorund dos caracteres
	wbkgd(wGame, COLOR_PAIR(4));      // define backgound dos espaço vazio
	scrollok(wGame, TRUE);
	keypad(wGame, TRUE);  

	wrefresh(wGame);
	wrefresh(wComands);
	wrefresh(wGeneral);
	wrefresh(wInfo);

	if(player.accepted == 1) {		//normal player
		//printf("Normal player\n"); 

		playData.player = &player;
		playData.stop = &stop;
		playData.window = wComands;
		int fdWrEngine = open(FIFO_ENGINE_GAME, O_WRONLY);
		playData.fd = fdWrEngine;
		
		GAME game;

		readMap(fdRdEngine,wGame,wInfo);

		if(pthread_create(&threadPlayID, NULL, &threadPlay, &playData)) {
			perror("Error creating threadPlay\n");
			//send player to engine to take him out of the game
		}
		//printf("Created threadPlay\n");
		recMSGData.window = wGeneral;
		recMSGData.stop = &stop;

		if(pthread_create(&threadRecMSGID, NULL, &threadRecMessages, &recMSGData)) {
			perror("Error creating threadRecGame\n");
			//send player to engine to take hi out of the game
		}
		//printf("Created threadRecMessages\n");
	} else {
		readMap(fdRdEngine,wGame,wInfo);
	}
	

	//thread rec game		
	recGameData.stop = &stop;
	recGameData.window = wGame;
	recGameData.wInfo = wInfo;
	recGameData.fdRdEngine = fdRdEngine;

	
	if(pthread_create(&threadRecGameID, NULL, &threadRecGame, &recGameData)) {
		perror("Error creating threadRecGame\n");
		//send player to engine to take hi out of the game
	}

	pthread_join(threadRecGameID, NULL);
	pthread_join(threadRecMSGID, NULL);
	pthread_join(threadPlayID, NULL);
   	endwin();
   	return 0;
}

void resizeHandler(int sig) { //compor nesta função
    refresh();
}

int keyboardCmdGameUI(PLAYER *player, WINDOW *wComands) {
    char str1[10], str2[30], str3[100];
    char cmd[200], msg[200];
	werase(wComands);
	box(wComands,0,0);

	wrefresh(wComands);
	mvwprintw(wComands,1,1, "Command: ");

	fflush(stdout);
	wscanw(wComands," %200[^\n]", cmd);
	
	if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {
		strcpy(player->command, str1);
		strcpy(player->personNameMessage, str2);
		strcpy(player->message, str3);
		//wprintw(window,"\%s, %s, %s\n", str1, str2, str3);
		//wclear(wComands);
		return 2;
	} else {
		if(strcmp(cmd, "players") == 0) {
			strcpy(player->command, cmd);
			return 1;
		}else if(strcmp(cmd, "exit") == 0) {
			strcpy(player->command, cmd);
			return -1;
		} else {
			return 0;
		}
	}
}

void *threadPlay(void *data) { 
	PLAYDATA *plData = (PLAYDATA *) data;
	const char *p;
	int ch;
	int ret;
	int size;

	p = NULL;

/*
	int fdWrEngine = open(FIFO_ENGINE_GAME, O_RDWR);
    if(fdWrEngine == -1) {
        perror("Error openning engine game fifo\n"); 
    }
*/
	//mkfifo to receive the PID of player from the engine
	char pipeNamePIDPlayer[30];

	sprintf(pipeNamePIDPlayer, FIFO_PID_MSG, getpid());

	if(mkfifo(pipeNamePIDPlayer, 0666) == -1) {
		perror("\nError creating pid message fifo\n");
	}

	keypad(plData->window, TRUE);
	
    while (plData->stop) {
		wmove(plData->window, 1, 1);
		size = 0;
		ret = 0;
       	ch = wgetch(plData->window);  // MUITO importante: o input é feito sobre a janela em questão
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
         	echo();          // Volta a ligar o echo para se ver o que se está
         	nocbreak();      // a escrever. Idem input baseado em linha + enter

         	ret = keyboardCmdGameUI(plData->player, plData->window);

			if(ret == -1) {
				//send player to engine to kick himself
				plData->player->move = -3;
			} else if(ret == 1) {
				plData->player->move = -1;
			} else if(ret == 2) {
				plData->player->move = -2;
				//send player to engine and wait to receive the pipe name of the player, 
				//if the player doesnt exist the pipe name will be "error" and he will continue
				size = write (plData->fd, plData->player, sizeof(PLAYER));

				MESSAGE msg;
				//mvprintw(1,1,"Estou a escuta neste pipe: %s", pipeNamePIDPlayer);
				refresh();
				int fdRdEngPIDPlayer = open(pipeNamePIDPlayer, O_RDONLY);
				if(fdRdEngPIDPlayer == -1) {
					perror("Error openning pid message fifo\n"); 
				}

				size = read(fdRdEngPIDPlayer, &msg, sizeof(msg));
				close(fdRdEngPIDPlayer);
				if (size > 0){
				//mvprintw(2,1,"Recebi este nome de pipe %s, e este size %d" ,msg.pipeName,size);

				refresh();
				if(strcmp(msg.pipeName, "error") == 0)
					break;

				
				strcpy(msg.msg, plData->player->message);
				strcpy(msg.namePlayerSentMessage, plData->player->name);
				//write to player pipe name
				int fdWrPlayer = open(msg.pipeName, O_RDWR);
				if(fdWrPlayer == -1) {
					perror("Error openning private message fifo\n"); 
				}
				//mvprintw(4,1,"Antes da escrita");

				size = write (fdWrPlayer, &msg, sizeof(MESSAGE));
				//mvprintw(5,1,"Depois da escrita");
				close(fdWrPlayer);
				//printf("Sent: Player %s sent %s e o tamanho [%d]\n", msg.namePlayerSentMessage, msg.msg, size);
}		
			}

         	noecho();        // Volta a desligar o echo das teclas premidas
         	cbreak();        // e a lógica de input em linha+enter no fim
         	break;
      	case 'q':
         	*plData->stop = 1;
         	break;
    	}
		if(plData->player->move != -2) {
			//send player to engine ENGINE FIFO GAME
			size = write (plData->fd, plData->player, sizeof(PLAYER));
			//printf("Sent: %s com a move %d e o tamanho [%d]\n", plData->player->name, plData->player->move, size);
		}

      	if (p != NULL) {
         	p = NULL;
         	wrefresh(plData->window); 
      	}
   	}
	pthread_exit(NULL);
}

void readMap(int fdRdEngine,WINDOW * wGame, WINDOW* wInfo){
	GAME game;
	int size = 0;

	size = read (fdRdEngine, &game, sizeof(GAME));
	
	printmap(game, wGame, wInfo);

	//change the window
	wrefresh(wGame);
	wrefresh(wInfo);
}

void *threadRecGame(void *data) {
	RECGAMEDATA *recGData = (RECGAMEDATA *) data;
	//printf("Inside threadRecGame\n");
	int size = 0;
	while(recGData->stop) {
		wrefresh(recGData->window);
		box(recGData->window,0,0);

		//read do jogo do engine	GAMEUI FIFO PID meu
		readMap(recGData->fdRdEngine,recGData->window,recGData->wInfo);
	}
	//printf("Outside threadRecGame\n");
	pthread_exit(NULL);
}

void *threadRecMessages(void *data) {
	RECMSGDATA *recMSGData = (RECMSGDATA *) data; 
	MESSAGE msg;
	char pipeNamePrivMSG[30];
	int size = 0;
	//printf("Inside threadRecMessages\n"); 123
	//mkfifo PRIVATE MSG PID
	sprintf(pipeNamePrivMSG, FIFO_PRIVATE_MSG, getpid());

	if(mkfifo(pipeNamePrivMSG, 0666) == -1) {
		perror("\nError creating private message fifo\n");
	}

	//open fd read
	int fdRdPlayerMSG = open(pipeNamePrivMSG, O_RDWR);
	if(fdRdPlayerMSG == -1) {
		perror("Error openning private message fifo\n"); 
	}

	while(recMSGData->stop) {
		//read from pipe PRIVATE MSG PID
		size = read (fdRdPlayerMSG, &msg, sizeof(MESSAGE));
		//show player the msg from the other player
		werase(recMSGData->window);
		box(recMSGData->window,0,0);

		wrefresh(recMSGData->window);
		mvwprintw(recMSGData->window,1,1, "Player %s sent you: %s", msg.namePlayerSentMessage, msg.msg);


		wrefresh(recMSGData->window);
		refresh();
	}
	close(fdRdPlayerMSG);
	unlink(pipeNamePrivMSG);
	
	//printf("Outside threadRecMessages\n");
	pthread_exit(NULL);

}

void printmap(GAME game, WINDOW* wGame, WINDOW * wInfo){

	init_pair(1, COLOR_YELLOW, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_RED);

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
	
	mvwprintw(wInfo,1,1,"Moving Obstacle: %d",game.nObs);
	mvwprintw(wInfo,2,1,"Rocks: %d",game.nRocks);
	mvwprintw(wInfo, 3, 1, "Players: ");
	
	for(int i= 0;i<game.nPlayers;i++){
		//mvwprintw(wInfo,3+i,1,"%s",game.players[i].name);
		 wprintw(wInfo, "%s", game.players[i].name);

        // Add a comma and space if there are more players
        if (i < game.nPlayers - 1) {
            wprintw(wInfo, ", ");
        }
	}

	mvwprintw(wInfo, 4, 1, "Time: %d", game.timeleft);

	for(int i= 0;i<game.nPlayers;i++){
		if(game.players[i].pid == getpid()) {
			mvwprintw(wInfo, 5, 1, "Score: %d", game.players[i].score);
			break;
		}		
	}
	
}