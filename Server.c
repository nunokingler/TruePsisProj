#include <stdio.h>

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <semaphore.h>
#include <signal.h>

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
	colour colour_players[MAX_PLAYER];
	char * str_players[MAX_PLAYER];
    sem_t sem_board;
    sem_t sem_server;
} * board_state;

typedef struct ThrdIn {
    int client;
    board_state  b;
} * thread_init;

int * fd_interruprt;
void intHandler(int Sigint) {
    close(*fd_interruprt);
    exit(100);
}
int init_boardState(board_state * d ,int dim){
    (*d)=malloc(sizeof(struct BoardState));
    (*d)->playerNumber=0;

    (*d)->size=dim;
    for(int i=0; i<MAX_PLAYER;i++){
        (*d)->str_players[i]=malloc((sizeof(char)*MAX_CHAR));
        strcpy((*d)->str_players[i],"");

        (*d)->colour_players[i]=malloc(sizeof(struct RGBCOLOR));
        colourSet((*d)->colour_players[i],0,0,0);
        (*d)->players[i]=-1;
    }
    init_board(dim);
    (*d)->start=1;
    if (sem_init(&((*d)->sem_server), 0, 1) == -1 || sem_init(&((*d)->sem_board), 0, 1) == -1){
        perror("sem_init error");
        exit(-1);
    }
}
void sendToEveryone(board_state b,serverMessage message){
    if(b==NULL)
        return;
    for(int i=0;i<MAX_PLAYER;i++){
        if(strcmp(b->str_players[i],"")!=0){
            //send(b->players[i],message,sizeof(struct Server_Message),0);
            write(b->players[i],message,sizeof(struct Server_Message));
        }
    }
}
void firstSquareTurn(board_state b,int x,int y,colour playerColour){
    if(b==NULL || playerColour==NULL)
        return;
    sem_wait(&b->sem_server);
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=0;
    message->x=x;
    message->y=y;
    colourCopy(&message->colour,playerColour);
    strcpy(message->Card,get_board_place_str(x,y));
    message->newValue=getBoardState(x,y);
    sendToEveryone(b,message);

    sem_post(&b->sem_server);
    free(message);
}
void secondSquareUpdate(board_state b,int x1,int y1,int x2,int y2,colour playerColour){
    if(b==NULL || playerColour==NULL)
        return;
    sem_wait(&b->sem_server);
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=0;
    message->x=x1;
    message->y=y1;
    colourCopy(&message->colour,playerColour);
    strcpy(message->Card,get_board_place_str(x1,y1));
    message->newValue=getBoardState(x1,y1);

    sendToEveryone(b,message);//sent message for first square

    message->x=x2;
    message->y=y2;
    sendToEveryone(b,message);
    sem_post(&b->sem_server);
    free(message);
}
int ver_win(board_state  b, char player) {
	int  points[b->size];
	for (int i = 0; i < b->size; i++)
		for (int j = 0; j < b->size; j++) {
			if (get_board_place_str(i,j)[2] == '\0')
				return -1;
		}
	return 1;
}

int main() {
	struct sockaddr_in server_addr;
    board_state b;
	pthread_t  threads[MAX_PLAYER];

    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);

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
    fd_interruprt=malloc(sizeof(int));
    *fd_interruprt=sock_fd;
    int * clientSock = malloc(sizeof(int));
	listen(sock_fd, 5);//Listen

	while (1) {
		printf("Waiting for players\n");

		*clientSock=accept(sock_fd, NULL, NULL);        //accept
		printf("1 - client connected\n");
		sem_wait(&b->sem_server);
        for(int i=0;i<MAX_PLAYER;i++){
            if(strcmp(b->str_players[i],"")==0){
                thread_init threadInit= malloc(sizeof(struct ThrdIn));
                threadInit->client=i;
                threadInit->b=b;
                int pthreaderr;
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
	}
	free(clientSock);
	for(int i=0;i<MAX_PLAYER;i++){
	    if(b->str_players!=NULL){
	        close(b->players[i]);
	    }
	}
}




void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    thread_init init= (thread_init)socket_desc;
    board_state b =  init->b;
    sem_wait(&b->sem_server);   //Server sem wait
    int playerNmbr=init->client;
    int sock = b->players[playerNmbr];

    colour playerColour=malloc(sizeof(struct RGBCOLOR));//Store colour in local variable to not use server semaphore
    colourCopy(playerColour,b->colour_players[playerNmbr]);

    sem_post(&b->sem_server);   //Sem Post
    free(socket_desc);

    clientMessage climen=malloc(sizeof(struct Client_Message));
    int read_size=recv(sock , climen , sizeof(struct Client_Message) , 0);
    //Receive a message from client
    while( read_size> 0 )
    {
        if(read_size== sizeof(struct Client_Message)){
            printCliMessage(climen);

            switch (climen->code){
                case 0:
                    if(climen->x >= 0 && climen->x < BOARD_SIZE
                        &&climen->y >= 0 && climen->y < BOARD_SIZE){
                        sem_wait(&b->sem_board);
                        play_response pr = board_play(climen->x,climen->y,playerNmbr);

                        switch (pr->code){
                            case 0://filled
                                break;
                            case 1://first play
                                firstSquareTurn(b,climen->x,climen->y,playerColour);
                                break;
                            case 3:
                                sem_wait(&b->sem_server);
                                //TODO send message to everyone that the game is over
                                sem_post(&b->sem_server);
                                break;
                            case 2://Case miss second and hit second call the same funcition
                            case -2:
                                secondSquareUpdate(b,pr->play1[0],pr->play1[1],pr->play2[0],pr->play2[1],playerColour);
                                if(pr->code==-2){
                                    sleep(2);//TODO Sleep
                                    //TODO set sleep variable
                                }
                                break;
                            default:break;
                        }
                        print_Board();
                        sem_post(&b->sem_board);
                        freePlayResponse(pr);
                    }
                    break;
                case 1:
                    sem_wait(&b->sem_server);
                    strcpy(b->str_players[playerNmbr],climen->str_play1);
                    sem_post(&b->sem_server);
                    break;
                default:break;
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

