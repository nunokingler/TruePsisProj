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



typedef struct BoardState {
	int size;
	int start;
	int playerNumber;
	int players[MAX_PLAYER];//TODO investigate into making these client structures
	colour colour_players[MAX_PLAYER];
	char * str_players[MAX_PLAYER];
    sem_t sem_board;
    sem_t sem_server;
} * board_state;


typedef struct Client {
    int cliNmbr,sock;
    colour cliColour;
    char cliName[MAX_CHAR];
    int state;//0- no play made
                //1 - play 1 mad
    int timeout;
} * client;

typedef struct ThrdIn {
    int client;
    board_state  b;
} * thread_init;
typedef struct TimerParam {
    client cl;
    board_state  b;
} * timerThreadParam;


void * connection_handler(void *socket_desc);
void * clientTimer(void * param);
void * gameStartTimer(void * board);

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
    (*d)->start=0;
    if (sem_init(&((*d)->sem_server), 0, 1) == -1 || sem_init(&((*d)->sem_board), 0, 1) == -1){
        perror("sem_init error");
        exit(-1);
    }
}
void sendErrorCode(client cli,int code){
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=code;
    message->x=0;
    message->y=0;
    colourCopy(&message->colour,cli->cliColour);
    strcpy(message->Card,"");
    message->newValue = -1;
    write(cli->sock,message,sizeof(struct Server_Message));
    free(message);
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
void sendGameState(board_state b,int state){//MAKE SURE SERVER SEM IS CLOSED WHEN USING THIS ONE
    if(b==NULL)
        return;
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=state;//3= start, 4= win
    message->x=-1;
    message->y=-1;
    message->colour.r=0;
    message->colour.g=0;
    message->colour.b=0;
    strcpy(message->Card,"");//TODO get winner and send it on this string
    message->newValue=-1;
    sendToEveryone(b,message);
    free(message);
}

void setNewGameStart(board_state b){
    pthread_t thred;
    int pthreaderr;
    sem_wait(&b->sem_server);
    sendGameState(b,4);
    b->start=0;
    sem_post(&b->sem_server);
    do{
        pthreaderr=pthread_create( &thred , NULL ,  gameStartTimer , (void*) b);
        if(pthreaderr<0)
            perror("could not create thread");
    }while (pthreaderr<0);

}
void * gameStartTimer(void * board){
    if(board==NULL)
        return NULL;

    board_state b= (board_state) board;
    sleep(30);//TODO problem, if players play again during this timeout and finish the game the third game will restart Faster

    sem_wait(&b->sem_board);
    sem_wait(&b->sem_server);
    if(b->start==0) {
        init_board(b->size);
        b->start = 1;
        sendGameState(b,3);
    }
    sem_post(&b->sem_board);
    sem_post(&b->sem_server);
}
void sendOneSquareTurn(board_state b, int x, int y, colour playerColour){
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
    strcpy(message->Card,get_board_place_str(x2,y2));
    message->newValue=getBoardState(x2,y2);
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
                //TODO give colour to the player
                strcpy(b->str_players[i] , "Filled");
                printf("Added new player");
                if(b->playerNumber==2) {
                    b->start = 1;
                    sendGameState(b,3);
                }
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
    client cli= malloc(sizeof(struct Client));
    sem_wait(&b->sem_server);   //Server sem wait
    cli->cliNmbr=init->client;
    cli->sock = b->players[cli->cliNmbr];
    strcpy(cli->cliName,b->str_players[cli->cliNmbr]);
    cli->cliColour=malloc(sizeof(struct RGBCOLOR));//Store colour in local variable to not use server semaphore
    colourCopy(cli->cliColour,b->colour_players[cli->cliNmbr]);
    cli->state=0;
    cli->timeout=0;

    sem_post(&b->sem_server);   //Sem Post
    free(socket_desc);
/*
    struct timeval timeout;
    timeout.tv_sec = 1000;
    timeout.tv_usec = 0;

    if (setsockopt (cli->sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                    sizeof(timeout)) < 0)
        perror("setsockopt failed\n");*/

    clientMessage climen=malloc(sizeof(struct Client_Message));
    int read_size=recv(cli->sock , climen , sizeof(struct Client_Message) , 0);
    int wrong_choice=0;
    //Receive a message from client
    while( read_size> 0 )
    {
        if(read_size== sizeof(struct Client_Message)){
            //printCliMessage(climen);
            cli->timeout=0;
            sem_wait(&b->sem_server);
            int gameStart=b->start;
            sem_post(&b->sem_server);
            if(gameStart > 0){
                switch (climen->code){
                    case 0:
                        if(climen->x >= 0 && climen->x < BOARD_SIZE
                            &&climen->y >= 0 && climen->y < BOARD_SIZE){
                            sem_wait(&b->sem_board);
                            play_response pr = board_play(climen->x,climen->y,cli->cliNmbr);

                            switch (pr->code){
                                case 0://filled
                                    printf("The square %d-%d is filled player %s,%d",climen->x,climen->y,cli->cliName,cli->cliNmbr);
                                    sendErrorCode(cli,1);
                                    break;
                                case 1://first play
                                    sendOneSquareTurn(b, climen->x, climen->y, cli->cliColour);
                                    int pthreadRet;
                                    cli->state=1;
                                    pthread_t threadID;
                                    timerThreadParam param = malloc(sizeof(struct TimerParam));
                                    param->b=b;
                                    param->cl=cli;
                                    do{
                                        pthreadRet=pthread_create( &threadID , NULL ,  &clientTimer , (void*) param);
                                    }while(pthreadRet<0);
                                    break;
                                case 3:
                                case 2://Case miss second and hit second call the same funcition
                                case -2:
                                    cli->state=0;
                                    cli->timeout++;
                                    secondSquareUpdate(b,pr->play1[0],pr->play1[1],pr->play2[0],pr->play2[1],cli->cliColour);
                                    if(pr->code==-2){
                                        wrong_choice=1;
                                    }
                                    else
                                        if(pr->code==3)
                                            setNewGameStart(b);
                                    break;
                                default:break;
                            }
                            print_Board();
                            sem_post(&b->sem_board);
                            if(wrong_choice){
                                sleep(2);
                                wrong_choice=0;
                                sem_wait(&b->sem_board);

                                unlockSquare(pr->play1[0],pr->play1[1],pr->play2[0],pr->play2[1]);
                                secondSquareUpdate(b,pr->play1[0],pr->play1[1],pr->play2[0],pr->play2[1],cli->cliColour);

                                sem_post(&b->sem_board);
                            }
                            freePlayResponse(pr);
                        }
                        break;
                    case 1:
                        sem_wait(&b->sem_server);
                        strcpy(b->str_players[cli->cliNmbr],climen->str_play1);
                        sem_post(&b->sem_server);
                        break;
                    default:break;
                }
            }
            else{
                sendErrorCode(cli,2);
            }
        }
        read_size=recv(cli->sock, climen , sizeof(struct Client_Message) , 0);
    }
    if(read_size <=0)
    {
        sem_wait(&b->sem_board);   //Server sem wait
        if(read_size==0)
            printf("Client %s disconnected because of timeout",b->str_players[cli->cliNmbr]);
        if(read_size==-1)
            perror("recv failed");
        close(cli->sock);

        sem_wait(&b->sem_server);
        strcpy(b->str_players[cli->cliNmbr],"");
        b->players[cli->cliNmbr]=0;
        b->playerNumber--;
        if (b->playerNumber<2)
            b->start=0;
        sem_post(&b->sem_server);
        int removalNumber=0;

        int * toRemove=removePlayer(cli->cliNmbr,&removalNumber);   //remove player taken cards (reflip them over)
        free(cli);
        if(toRemove!=NULL){
            for(int i=0;i<removalNumber;i++)
                sendOneSquareTurn(b, toRemove[i * 2], toRemove[i * 2 + 1], cli->cliColour);
        }
        print_Board();
        free(toRemove);
        sem_post(&b->sem_board);   //Server sem wait
    }
    return 0;
}
void * clientTimer(void *param){
    if(param==NULL)
        return NULL;
    timerThreadParam tim = (timerThreadParam) param;
    client cl=tim->cl;
    board_state b= tim->b;
    int oldTimeout= ((client)cl)->timeout,* pointer;
    sleep(5);
    if(((client)cl)->timeout==oldTimeout && ((client)cl)->state==1) {
        pointer=removeChoice(((client)cl)->cliNmbr);
        if(pointer!=NULL){
            sendOneSquareTurn(b, pointer[0],pointer[1],((client)cl)->cliColour);
            free(pointer);
        }
    }
    free(param);
}

