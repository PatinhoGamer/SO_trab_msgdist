#include "cliente.h"


ClientConfig cfg;

int main(int argc,char* argv[]){


	if(!isServerRunning()){
		printf("Server not running\nExiting\n");
		exit(0);
	}
	
	signal(SIGINT,shutdown);
	printf("%d\n",getpid());
	printf("Username: \n");
	scanf("%s",cfg.username);

	//Criar fifo para servidor enviar coisas para cliente
	// Enviar PID, username e talvez path para FIFO
	// criar fifo em /tmp/
	sprintf (cfg.fifoPath,"/tmp/%d",getpid());
	int result = mkfifo(cfg.fifoPath,0666);
	if(result != 0) {
		fprintf(stderr,"[ERROR]Creating listener fifo");
	}

	cfg.fifo = open(cfg.fifoPath,O_RDWR);
	printf("Starting \n");
	if(cfg.fifo == -1 ){
		printf("fifo creation error (FIFO)\n");
		printf("Error: %d\n",errno);
		exit(0);
	}

	// printf("ola");
	cfg.server = open(LISTENER_PATH,O_RDWR);
	{
		NewClientInfo newClient;
		strcpy(newClient.username,cfg.username);
		newClient.pid = getpid();
		strcpy(newClient.pathToFifo,cfg.fifoPath);

		sendToServer(NEW_USER,&newClient,sizeof(NewClientInfo));
	}
	//pause();
	// Criar thread para receber info do servidor
	pthread_t listenerThread;
	pthread_create(&listenerThread,NULL,fifoListener,(void*)NULL);

	int menu_ret = 1, menu_ret2 = 1;
    char alts[][100] = {{" Write Message"},                 /* Every menu needs an */
                         {"Info needed"},
                         {"Subscribe to Topic"}, 
                         {"Unsubscribe from Topic"},        /* array like these to */
                         {"Exit"},};                  /* hold the entries.   */
    char alts2[][100] = {{"List Topics"},
                         {"List Titles in Topic"},
                         {"Read Message in Topic"},
                         {"Return"},};

    setlocale (LC_CTYPE, "");

    initscr();                  /* Most of the below initialisers are */
    noecho();                   /* not necessary for this example.    */
    keypad (stdscr, TRUE);      /* It's just a template for a         */
    meta (stdscr, TRUE);        /* hypothetical program that might    */
    nodelay (stdscr, FALSE);    /* need them.                         */
    notimeout (stdscr, TRUE);
    raw();
    curs_set (0);

	int choice;

	while(menu_ret != 5){
		
		menu_ret = print_menu (2, 5, 5, 15,
                               cfg.username, alts, menu_ret);

		if (menu_ret == 1)  /* This is just an example program. */
        {                   /* You can't start an actual game.  */

			Message message;
			strcpy(message.username,cfg.username);
			char topic[20], titulo[100];
			char msg[1000];

			echo();
			mvprintw(6, 27,"Topico da menssagem: ");
			refresh();
			scanw("%s",message.topic);
			


			mvprintw(8, 27,"Titulo da menssagem: ");
			refresh();
			scanw("%s",message.title);

			//Message newMsg(username,topic,titulo,msg);

			mvprintw(10,27,"Mensagem: ");
			refresh();
			scanw("%s",message.body);

			sendToServer(NEW_MESSAGE,&message,sizeof(Message));
			mvprintw(12,27,"Mensagem enviada\n");
            getch();
        }
		if (menu_ret == 2) /* If you select load game, a new    */
        {                       /* menu will show up without erasing */
            do                  /* the main menu. */
            {
                menu_ret2 = print_menu (6, 34, 4, 15,
                                        "SELECT SLOT", alts2, 1);


				if (menu_ret2 == 1 ){

					// List Topics

				}
				if (menu_ret2 == 2 ){
					
					// List Titles in Topic

				}
				if (menu_ret2 == 3 ){
					
					// Read Message in Topic

				}
            }
            while (menu_ret2 != 4);
        }
		if(menu_ret == 3){

			mvprintw (5, 23, "Lol jk! This is a demo program.");
            mvprintw (6, 26, "Press any key to return.");
            getch();
		}
		if(menu_ret == 4){

			//Unsubscribe to Topic
		}

		if(menu_ret == 5){
			mvprintw(6, 34,"Exiting");
			shutdown(SIGINT);
		}

		erase(); 
		/*
		switch (choice)
		{
		case 0:{ // Exit
			printf("Exiting\n");
			shutdown(SIGINT);
			break;
		}
		case 1:{

			
			Message message;
			strcpy(message.username,cfg.username);
			char topic[20], titulo[100];
			char msg[1000];


			printf("Topico da menssagem: ");
			scanf("%s",message.topic);


			printf("Titulo da menssagem: ");
			scanf("%s",message.title);

				//Message newMsg(username,topic,titulo,msg);

			printf("Mensagem: ");
			scanf("%s",message.body);

			sendToServer(NEW_MESSAGE,&message,sizeof(Message));
			printf("Mensagem enviada\n");

			break;
		}
		case 2:{ // List Topics
			break;
		}
		case 3:{ // List Titles in Topic
			break;
		}
		case 4:{ // Read Message in Topic
			break;
		}
		case 5:{ // Subscribe to Topic
			break;
		}
		case 6:{// Unsubscribe to topic
			break;
		}
		default:
			printf("Invalid Option\n");
			break;
		}
		*/

	}


	/*initscr();
	cbreak();
	int nLines = 20,nCols = 100,y0 = 10,x0 = 10;
	WINDOW * win = newwin(nLines, nCols, y0, x0);
	wrefresh(win);
	while(1){pause();}*/
	endwin();
	return 0;
}


void* fifoListener(void* data){
	while(1){
		int const bufferSize = 8192;
		char buff[bufferSize];
		void* buffer = buff;

		int bCount = read(cfg.fifo, buffer, bufferSize);
		fprintf(stderr,"[INFO]Recebeu bytes : %d\n",bCount);
		Command* command = (Command*)buffer;
		buffer = buffer + sizeof(Command);

		if(command->cmd == SERVER_SHUTDOWN){
			printf("Server ");
			shutdown(SIGINT);
		}
		

	}
}

void shutdown(int signal){
	printf("Shuting Down\n");
	sendToServer(USER_LEAVING,NULL,0);

	close(cfg.server);
	close(cfg.fifo);
	unlink(cfg.fifoPath);
	exit(0);
}

int sendToServer(int cmd,void* other,size_t size){
	Command command;
	command.cmd = cmd;
	command.clientPid = getpid();
	command.structSize = size;
	
	Buffer buffer = joinCommandStruct(&command,other,size);
	int written = write(cfg.server,buffer.ptr,buffer.size);
	free(buffer.ptr);
	return written;
}