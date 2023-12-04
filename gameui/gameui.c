#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <error.h>
#include <sys/wait.h>
#include <time.h>
#include <ncurses.h>
#include "gameui.h"

#define ENGINE_FIFO "ENGINEFIFO"

#define TAM 20


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

int main(int argc, char *argv[]) {
    int lin, col, color, exit;
	char pipeName[50];
    WINDOW * window;
    int ch;
    const char * p;
    srand(time(NULL));  // p/ números aleatórios. não tem nada a ver com ncurses

    PLAYER player;
    
    
    if(argc != 2) {
        printf("\n[ERROR] Invalid number of arguments -> Sintax: <./gameui NAME>\n");
        exit(1);
    }
	
    
    strcpy(player.name, argv[1]);

	int fdWrInitEngine = open(ENGINE_FIFO, O_WRONLY);   //Abrir o fifo em modo de escrita bloqueante
	if(fdWrInitEngine == -1) {
		perror("Engine is not running yet");     
		return -1;
	}

	//send player to engine to see if he can enter the game
	int size = write (fdWrInitEngine, &player, sizeof(player));
	printf("\Sent: %s com o tamanho [%d]", player.name, size);


    //receive response from engine to see if the player can enter the game
	sprintf(pipeName, "GAMEUIFIFO_%d", getpid());
        if(mkfifo(pipeName, 0666) == -1) {
        perror("\nError creating gameui fifo\n");
        return -1;
    }
    int fdRdInitEngine = open(pipeName, O_RDWR);
    if(fdRdInitEngine == -1) {
        perror("Error openning gameui fifo\n"); 
    }

	int size = read (fdRdInitEngine, &player, sizeof(player));
	if(!player.accepted) {
		perror("There are already someone with your name or the time to enter the game ended\n"); 
		//unlink();
		//close();
		return -1;
	}


   initscr();  
   start_color();
   erase(); 

   init_pair(1, COLOR_RED, COLOR_BLACK);
   init_pair(2, COLOR_GREEN, COLOR_BLACK);
   init_pair(3, COLOR_BLUE, COLOR_BLACK);
   init_pair(4, COLOR_YELLOW, COLOR_BLUE);
   init_pair(5, COLOR_YELLOW, COLOR_BLACK);
   refresh(); 

    noecho();   // Desliga o echo porque a seguir vai ler telcas de direção não se pretende "ecoar" essa tecla no ecrã
    cbreak();   // desliga a necessidade de enter. cada tecla é lida imediatamente

  	attron(COLOR_PAIR(5));

    desenhaMoldura(20,50,6,15);
    window = newwin(20, 50, 6, 15);   
    wattrset(window, COLOR_PAIR(4));  // define foreground/backgorund dos caracteres
    wbkgd(window, COLOR_PAIR(4));      // define backgound dos espaço vazio
    scrollok(window, TRUE);
    keypad(window, TRUE);  

    werase(window);
    wrefresh(window);
    wprintw(window,"Teclas de direção e espaço e q para sair\n");
    wrefresh(window);

    exit = 0;
    p = NULL;
    while (exit != 1) {
       	ch = wgetch(window);  // MUITO importante: o input é feito sobre a janela em questão
       	switch (ch) {
       	case KEY_UP:
        	p = "up";
        	 break;
      	case KEY_DOWN:
      	   p = "down";
      	   break;
     	case KEY_LEFT:
     	    p = "left";
     	    break;
     	case KEY_RIGHT:
    	    p = "right";
    	    break;
   	   	case ' ':
         	echo();          // Volta a ligar o echo para se ver o que se está
         	nocbreak();      // a escrever. Idem input baseado em linha + enter

         	keyboardCmdGameUI(player, window);
         
         	noecho();        // Volta a desligar o echo das teclas premidas
         	cbreak();        // e a lógica de input em linha+enter no fim
         	break;
      	case 'q':
         	exit = 1;
         	break;
    	}
      	if (p != NULL) {
         	wprintw(window, "%s\n", p);   // reparar "w" inicial: imprime na janela
         	p = NULL;
         	wrefresh(window); 
      	}
   	}

   endwin();
   return 0;
}


void keyboardCmdGameUI(PLAYER player, WINDOW *window) {
    char str1[10], str2[30], str3[100];
    char cmd[200], msg[200];

	wprintw(window,"\nCommand: ");
	fflush(stdout);
	wscanw(window," %200[^\n]", cmd);
	//sprintf(msg,"output:[%s]", cmd);

	//fgets(cmd, sizeof(cmd), stdin);
	
	if (sscanf(cmd, "%s %s %[^\n]", str1, str2, str3) == 3 && strcmp(str1, "msg") == 0) {
		strcpy(player.command, str1);
		strcpy(player.personMessage, str2);
		strcpy(player.message, str3);
		//send player to engine
		//wprintw(window,"\%s, %s, %s\n", str1, str2, str3);
		wprintw(window,"\nVALID\n");
	} else {

		//cmd[ strlen( cmd ) - 1 ] = '\0';
		if(strcmp(cmd, "players") == 0) {
			strcpy(player.command, cmd);
			//send player to engine
			wprintw(window,"\nVALID\n");
		}else if(strcmp(cmd, "exit") == 0) {
			strcpy(player.command, cmd);
			wprintw(window,"\nVALID\n");
			//avisar o motor com um sinal;
			return;
		} else {
			wprintw(window,"\nINVALID\n");
		}
	}

}
