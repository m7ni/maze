#include "gameui.h"

void printmap(GAME game){
    clear();
    refresh();

    int width=16;
    int height=40;

	init_pair(1, COLOR_YELLOW, COLOR_BLUE);
	init_pair(2, COLOR_YELLOW, COLOR_RED);
        
	for(int i = 0 ; i < height ; i++) {
		for(int j=0 ; j<width ; j++){
			if(game.map[i][j]=='#'){
				attron(COLOR_PAIR(1));
				mvprintw(i,j,"#");
				attroff(COLOR_PAIR(1));
			}
			else if(game.map[i][j] == ' '){
				mvprintw(i,j," ");
			}
		}
	}
	
    refresh();
}

void desenhaMoldura(int alt, int comp, int lin, int col) {
	--lin;
	--col;
	alt+=2;
	comp+=2;
	// desenha barras verticais laterais
	for (int l=lin; l<=lin+alt-1; ++l) {
		mvaddch(l, col, '|');            // MoVe para uma posição e ADDiciona um CHar lá
		mvaddch(l, col+comp-1, '|');
	}
	// desenha as barras horizontais
	for (int c=col; c<=col+comp-1; ++c) {
		mvaddch(lin, c, '-');
		mvaddch(lin+alt-1, c, '-');
	}
	// desenha os cantos
	mvaddch(lin, col, '+');
	mvaddch(lin, col+comp-1, '+');
	mvaddch(lin+alt-1, col, '+');
	mvaddch(lin+alt-1,col+comp-1,'+');
	refresh();
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


int main(int argc, char *argv[]) {
    int lin, col, color, stop = 0;
	char pipeName[50];
    WINDOW * window;
    
    PLAYER player;
	PLAYDATA playData;
	pthread_t threadPlayID;
    
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

	player.pid = getpid();

	sprintf(pipeName, FIFO_GAMEUI, getpid());

	if(mkfifo(pipeName, 0666) == -1) {
		perror("\nError creating gameui fifo\n");
		return -1;
    }

    int fdRdEngine = open(pipeName, O_RDWR);
    if(fdRdEngine == -1) {
        perror("Error openning gameui fifo\n"); 
    }
	
	//send player to engine to see if he can enter the gamemm
	int size = write (fdWrInitEngine, &player, sizeof(player));
	printf("Sent: %s com o tamanho [%d]\n", player.name, size);

    //receive response from engine to see if the player can enter the game
	size = read (fdRdEngine, &player, sizeof(player));

	if(player.accepted == 0) {	//cannot play because there are already someone with that name
		printf("There are already someone with your name or the time to enter the game ended\n"); 
		//unlink(pipeName);
		//close(fdRdEngine);
		return -1;
	} else {

		initscr();  
		start_color();
		erase(); 

		init_pair(4, COLOR_YELLOW, COLOR_BLUE);
		init_pair(5, COLOR_YELLOW, COLOR_BLACK);
		refresh(); 

		noecho();   // Desliga o echo porque a seguir vai ler telcas de direção não se pretende "ecoar" essa tecla no ecrã
		cbreak();   // desliga a necessidade de enter. cada tecla é lida imediatamente

		attron(COLOR_PAIR(5));

		desenhaMoldura(20,50,6,15);
		window = newwin(30, 50, 6, 15);   
		wattrset(window, COLOR_PAIR(4));  // define foreground/backgorund dos caracteres
		wbkgd(window, COLOR_PAIR(4));      // define backgound dos espaço vazio
		scrollok(window, TRUE);
		keypad(window, TRUE);  

		werase(window);
		wrefresh(window);
		//wprintw(window,"Teclas de direção e espaço e q para sair\n");
		wrefresh(window);

		

		if(player.accepted == 2) {	//the player will only see the game, he cant do anything else
			printf("Time to enroll has ended, you will only be able to watch\n"); 
			 //thread rec game
			 
		} else if(player.accepted == 1) {		//normal player
			printf("Normal player\n"); 

			playData.player = &player;
			playData.stop = &stop;

			if(pthread_create(&threadPlayID, NULL, &threadPlay, &playData)) {
				perror("Error creating threadPlay\n");
				return -1;
			}
			//thread rec game
			GAME game;
			readFileMap(1, &game);
			printmap(game);

			pthread_join(threadPlayID, NULL);
			//unlink(pipeName);
			//close(fdRdEngine);
		}	
	}

   endwin();
   return 0;
}

int keyboardCmdGameUI(PLAYER *player, WINDOW *window) {
    char str1[10], str2[30], str3[100];
    char cmd[200], msg[200];

	wprintw(window,"\nCommand: ");
	fflush(stdout);
	wscanw(window," %200[^\n]", cmd);
	
	if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {
		strcpy(player->command, str1);
		strcpy(player->personNameMessage, str2);
		strcpy(player->message, str3);
		//wprintw(window,"\%s, %s, %s\n", str1, str2, str3);
		return 2;
		wprintw(window,"\nVALID\n");
	} else {
		if(strcmp(cmd, "players") == 0) {
			strcpy(player->command, cmd);
			wprintw(window,"\nVALID\n");
			return 1;
		}else if(strcmp(cmd, "exit") == 0) {
			strcpy(player->command, cmd);
			wprintw(window,"\nVALID\n");
			return -1;
		} else {
			wprintw(window,"\nINVALID\n");
			return 0;
		}
	}
}

void *threadPlay(void *data) { 
	PLAYDATA *plData = (PLAYDATA *) data;
	const char *p;
	int ch;
	int ret;

	p = NULL;

	//fdWrEngine

    while (plData->stop == 0) {
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
			} else if(ret == 1) {
				plData->player->move = -1;
			} else if(ret == 2) {
				plData->player->move = -2;
				//send player to engine and wait to receive the pipe name of the player, 
				//if the player doesnt exist the pipe name will be "error" and he will continue
				//write to player pipe name
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
		}

      	if (p != NULL) {
         	wprintw(plData->window, "%s\n", p);   // reparar "w" inicial: imprime na janela
         	p = NULL;
         	wrefresh(plData->window); 
      	}
   	}
	pthread_exit(NULL);
}

void *threadRecGame(void *data) {
	RECGAMEDATA *recGData = (RECGAMEDATA *) data;
	GAME game;
/*
	int fdRdEngine = open(pipeName, O_RDWR);
    if(fdRdEngine == -1) {
        perror("Error openning gameui fifo\n"); 
    }*/

	int size = 0;
	while(recGData->stop == 0) {
		

		//read do jogo do engine	GAMEUI FIFO PID meu
		//size = read (fdRdEngine, &game, sizeof(game));
		//mutex lock window

			//change the window

		//mutex unlock window

	}
	pthread_exit(NULL);
}

void *threadRecMessages(void *data) {
	RECMSGDATA *recMSGData = (RECMSGDATA *) data;
		
	//create pipe to receive the msg from player	PRIVATE MSG PID
	//mkfifo PRIVATE MSG PID
	//open fd read

	//while(recGData->stop == 0) {

		//read from pipe PRIVATE MSG PID
		
		//mutex lock window

		//show player the msg from the other player

		//mutex unlock window
		
		//close pipe

	//}
	pthread_exit(NULL);

}


