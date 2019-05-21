#include <stdio.h>

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <semaphore.h>

#include "gameConnection.h"
#include "board_library.h"

//#include "galo-config.h"

#define BOARD_SIZE 4

void *connection_handler(void *socket_desc);

typedef struct BoardState {
	int size;
	int start;
	int playerNumber;
	int players[MAX_PLAYER];
	char * str_players[MAX_PLAYER];
    sem_t sem_board;
    sem_t sem_server;
} * board_state;

typedef struct ThrdIn {
    int client;
    board_state  b;
} * thread_init;

int init_boardState(board_state * d ,int dim){
    (*d)=malloc(sizeof(struct BoardState));
    (*d)->playerNumber=0;
    //(*d)->players= malloc(MAX_PLAYER*sizeof(int));

    (*d)->size=dim;
    //(*d)->str_players = malloc(sizeof(char *) * MAX_PLAYER);
    printf("the size is %d",sizeof(char *) * MAX_PLAYER);
    for(int i=0; i<MAX_PLAYER;i++){
        (*d)->str_players[i]=malloc((sizeof(char)*MAX_CHAR));
        strcpy((*d)->str_players[i],"");
        (*d)->players[i]=-1;

    }
    init_board(dim);
    (*d)->start=1;
    if (sem_init(&((*d)->sem_server), 0, 1) == -1 || sem_init(&((*d)->sem_board), 0, 1) == -1){
        perror("sem_init error");
        exit(-1);
    }
}

int ver_win(board_state  b, char player) {
	int  points[b->size];
	for (int i = 0; i < b->size; i++)
		for (int j = 0; j < b->size; j++) {
			if (get_board_place_str(i,j)[2] == '\0');
				return -1;
		}
	return 1;
}

int mainServer() {
	struct sockaddr_in server_addr;
    board_state b;
	int msg_ret;
	int players_fd[2];
	pthread_t  threads[MAX_PLAYER];

    init_boardState(&b,BOARD_SIZE);
    printf("Printing Board\n");
    print_Board();
    printf("Printed Board\n");


	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);//creation

	if (sock_fd == -1) {
		perror("socket: ");
		exit(-1);
	}
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
	int err = bind(sock_fd, (struct sockaddr*) &server_addr, sizeof(server_addr));//bind
	if (err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n");
	listen(sock_fd, 5);                         //Listen
	while (1) {
		printf("Waiting for players\n");

		int * clientSock = malloc(sizeof(int));
		*clientSock=accept(sock_fd, NULL, NULL);        //accept
		printf("1 - client connected\n");
		sem_wait(&b->sem_server);
        for(int i=0;i<MAX_PLAYER;i++){
            if(strcmp(b->str_players[i],"")==0){
                thread_init threadInit= malloc(sizeof(struct ThrdIn));
                threadInit->client=i;
                threadInit->b=b;
                int pthreaderr;
                clientMessage  cl=malloc(sizeof(struct Client_Message));
    //            int read_size = recv(*clientSock , cl , sizeof(struct Client_Message) , 0);
                do{
                    pthreaderr=pthread_create( &threads[i] , NULL ,  connection_handler , (void*) threadInit);
                    if(pthreaderr<0)
                        perror("could not create thread");
                }while (pthreaderr<0);
                b->playerNumber++;
                b->players[i] = *clientSock;
                strcpy(b->str_players[i] , "Filled");
                printf("Added new player");
                break;
            }
        }
        sem_post(&b->sem_server);

            //  int player_2= accept(sock_fd, (struct sockaddr *) & client_addr, &size_addr);
		//write(players_fd[0], &players[0], sizeof(players[0]));
		//players_fd[1] = accept(sock_fd, NULL, NULL);
		//printf("1 - client connected\n");
		//write(players_fd[1], &players[1], sizeof(players[1]));
/*
		while (b->start) {
			write(players_fd[0], &b, sizeof(b));
			play_remote(&b, players[0], players_fd[0]);
			write(players_fd[0], &b, sizeof(b));

			write(players_fd[1], &b, sizeof(b));
			play_remote(&b, players[1], players_fd[1]);
			write(players_fd[1], &b, sizeof(b));
		}*/
	}
	for(int i=0;i<MAX_PLAYER;i++){
	    if(b->str_players!=NULL){
	        close(b->players[i]);
	    }
	}
}

void printCliMessage(clientMessage pMessage) {
    printf("ID=%d\n,x=%d\ny=%d\nString=%s\n",pMessage->code,pMessage->x,pMessage->y,pMessage->str_play1);
}


void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    thread_init init= (thread_init)socket_desc;
    board_state b =  init->b;
    sem_wait(&b->sem_server);   //Server sem wait
    int playerNmbr=init->client;
    int sock = b->players[playerNmbr];//*(b->players + init->client),playerNmbr=init->client;
    sem_post(&b->sem_server);   //Sem Post
    free(socket_desc);

    clientMessage climen=malloc(sizeof(struct Client_Message));
    char *message ;
    int read_size=recv(sock , climen , sizeof(struct Client_Message) , 0);
    //Receive a message from client
    while( read_size> 0 )
    {
        if(read_size== sizeof(struct Client_Message)){
            printCliMessage(climen);

            switch (climen->code){
                case 0:
                    sem_wait(&b->sem_board);
                    if(climen->x < 0 || climen->x >= BOARD_SIZE
                        ||climen->y < 0 || climen->y >= BOARD_SIZE){
                    board_play(climen->x,climen->y,playerNmbr);

                    }
                    sem_post(&b->sem_board);
                    break;
                case 1:
                    sem_wait(&b->sem_server);
                    strcpy(b->str_players[playerNmbr],climen->str_play1);
                    sem_post(&b->sem_server);
                    break;
            }
        }
        read_size=recv(sock , climen , sizeof(struct Client_Message) , 0);
    }
    if(read_size == 0)
    {
        sem_wait(&b->sem_server);   //Server sem wait
        printf("Client %s disconnected",b->str_players[playerNmbr]);
        close(sock);
        //TODO FREE STRUCTURE of message and client
        sem_post(&b->sem_server);   //Server sem wait

    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    return 0;
}