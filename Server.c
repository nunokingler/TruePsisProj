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



typedef struct Client {
    int cliNmbr,sock;
    colour cliColour;
    char cliName[MAX_CHAR];
    int state;//0- no play made
                //1 - play 1 mad
    int timeout;
} * client;

typedef struct BoardState {
    int size;
    int start;
    int playerNumber;
    colour colourPallete[MAX_PLAYER];
    client clients[MAX_PLAYER];
    boardLibrary board;
    sem_t sem_board;
    sem_t sem_server;
} * gameState;

typedef struct ThrdIn {
    client client;
    gameState  b;
} * thread_init;
typedef struct TimerParam {
    client cl;
    gameState  b;
} * timerThreadParam;


void * connection_handler(void *socket_desc);
void * clientWrongTimer(void *param);
void * clientPickTimer(void *param);
void * gameStartTimer(void * board);
void killClient(gameState b, client cli);


int * fd_interruprt;
void intHandler(int Sigint) {
    close(*fd_interruprt);
    exit(100);
}
int init_boardState(gameState * d ,int dim){
    (*d)=malloc(sizeof(struct BoardState));
    (*d)->playerNumber=0;

    (*d)->size=dim;
    for(int i=0; i<MAX_PLAYER;i++){
        (*d)->clients[i]=NULL;
        (*d)->colourPallete[i]=malloc(sizeof(struct RGBCOLOR));
    }
    colourSet((*d)->colourPallete[0],0,255,0);// green
    colourSet((*d)->colourPallete[1],193,0,193);// purple
    colourSet((*d)->colourPallete[2],255,255,0);// yellow
    colourSet((*d)->colourPallete[3],0,0,255);// blue
    colourSet((*d)->colourPallete[4],0,255,255);// cyan blue
    colourSet((*d)->colourPallete[5],0,102,0);// dark green
    colourSet((*d)->colourPallete[6],0,0,102);// dark blue
    colourSet((*d)->colourPallete[7],255,0,127);// pink
    colourSet((*d)->colourPallete[8],255,128,0);// orange
    colourSet((*d)->colourPallete[9],102,178,255);// light blue

    (*d)->board= init_board(dim);
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
void sendToEveryone(gameState b,serverMessage message){
    if(b==NULL)
        return;
    for(int i=0;i<MAX_PLAYER;i++){
        if(b->clients[i]!=NULL && b->clients[i]->state!=-1){
            //send(b->players[i],message,sizeof(struct Server_Message),0);
            signal(SIGPIPE,SIG_IGN);
            write(b->clients[i]->sock,message,sizeof(struct Server_Message));
        }
    }
}
void sendGameState(gameState b,int state){//MAKE SURE SERVER SEM IS CLOSED WHEN USING THIS ONE
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

void setNewGameStart(gameState b){
    pthread_t thred;
    int pthreaderr;
    sem_wait(&b->sem_server);
    sendGameState(b,4);
    b->start=-1;
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

    gameState b= (gameState) board;
    sleep(10);

    sem_wait(&b->sem_board);
    sem_wait(&b->sem_server);
    if(b->start==-1) {
        init_board(b->size);
        b->start = 1;
        sendGameState(b,3);
        print_Board(b->board);
        printf("Let a new game begin!\n");
    }
    sem_post(&b->sem_board);
    sem_post(&b->sem_server);
}
void givePlayerBoardstate(gameState b, client cli) {
    if(b==NULL ||cli==NULL)
        return;
    sem_wait(&b->sem_board);
    int addr,* number=&addr,* to_send=getBoardState(b->board,number);
    sem_wait(&b->sem_server);
    serverMessage men=malloc(sizeof(struct Server_Message));
    men->code=7;men->newValue=b->size;
    write(cli->sock,men,sizeof(struct Server_Message));
    if(b->start)
        sendErrorCode(cli,3);
    if(number>0 && to_send!= NULL){
        if(men==NULL){
            return;
        }
        men->code=0;
        strcpy(men->winner,"");
        for(int i=0;i<*number;i++){
            strcpy(men->Card,get_board_place_str(b->board,to_send[i * 2],to_send[i * 2+1]));
            men->newValue=getBoardPlaceState(b->board,to_send[i * 2],to_send[i * 2+1]);
            colourCopy(&men->colour,b->colourPallete[getBoardPlacePlayer(b->board,to_send[i * 2],to_send[i * 2+1])]);
            men->x=to_send[i * 2];
            men->y=to_send[i * 2+1];
            write(cli->sock,men,sizeof(struct Server_Message));
        }
    }
    cli->state=0;
    sem_post(&b->sem_server);
    sem_post(&b->sem_board);
    free(men);
}
void sendOneSquareTurn(gameState b, int x, int y, colour playerColour){
    if(b==NULL || playerColour==NULL)
        return;
    sem_wait(&b->sem_server);
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=0;
    message->x=x;
    message->y=y;
    message->newValue= getBoardPlaceState(b->board, x, y);
    if(message->newValue!=0){
        colourCopy(&message->colour,playerColour);
        strcpy(message->Card,get_board_place_str(b->board,x,y));
    }
    else{
        colourSet(&message->colour,255,255,255);
        strcpy(message->Card,"");
    }

    sendToEveryone(b,message);

    sem_post(&b->sem_server);
    free(message);
}
void secondSquareUpdate(gameState b,int x1,int y1,int x2,int y2,colour playerColour){
    if(b==NULL || playerColour==NULL)
        return;
    sem_wait(&b->sem_server);
    serverMessage message= malloc(sizeof(struct Server_Message));
    message->code=0;
    message->x=x1;
    message->y=y1;
    message->newValue= getBoardPlaceState(b->board, x1, y1);
    if(message->newValue!=0){
        colourCopy(&message->colour,playerColour);
        strcpy(message->Card,get_board_place_str(b->board,x1,y1));
    }
    else{
        colourSet(&message->colour,255,255,255);
        strcpy(message->Card,"");
    }

    sendToEveryone(b,message);//sent message for first square

    message->x=x2;
    message->y=y2;
    message->newValue= getBoardPlaceState(b->board, x2, y2);
    if(message->newValue!=0){
        colourCopy(&message->colour,playerColour);
        strcpy(message->Card,get_board_place_str(b->board,x2,y2));
    }
    else{
        colourSet(&message->colour,255,255,255);
        strcpy(message->Card,"");
    }
    message->newValue= getBoardPlaceState(b->board, x2, y2);
    sendToEveryone(b,message);
    sem_post(&b->sem_server);
    free(message);
}
client initClient(int sock,int number, colour col){
    client cli=malloc(sizeof(struct Client));

    cli->sock=sock;
    cli->timeout=0;
    cli->cliColour=col;
    strcpy(cli->cliName,"Filled");
    cli->state=-1;
    cli->cliNmbr=number;

    return cli;
}

int main(int argc, char** argv) {
    if(argc<2){
        printf("Usage %s BoardSize",argv[0]);
        perror("");
    }
    int size=atoi(argv[1]);
    if(size<1 || size%2!=1)
        perror("Boardsize must be an EVEN number GREATER than 0");

	struct sockaddr_in server_addr;
    gameState b;
	pthread_t  threads[MAX_PLAYER];

    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);

    //init_boardState(&b,BOARD_SIZE);//TODO this should be a command line argument
    init_boardState(&b,size);

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
    print_Board(b->board);

    fd_interruprt=malloc(sizeof(int));
    *fd_interruprt=sock_fd;
    int clientSock;
	listen(sock_fd, 5);//Listen
	while (1) {
		printf("Waiting for players\n");

		clientSock=accept(sock_fd, NULL, NULL);        //accept
		sem_wait(&b->sem_server);
        for(int i=0;i<MAX_PLAYER;i++){
            if(b->clients[i]==NULL){
                b->clients[i]=initClient(clientSock,i,b->colourPallete[i]);
                printf("%d - client connected\n",i);
                thread_init threadInit= malloc(sizeof(struct ThrdIn));
                threadInit->client=b->clients[i];
                threadInit->b=b;
                int pthreaderr;
                do{
                    pthreaderr=pthread_create( &threads[i] , NULL ,  connection_handler , (void*) threadInit);
                    if(pthreaderr<0)
                        perror("could not create thread");
                }while (pthreaderr<0);
                b->playerNumber++;
                if(b->playerNumber==2 && b->start==0) {
                    b->start = 1;
                    sendGameState(b,3);
                }
                break;
            }
        }
        sem_post(&b->sem_server);
	}
	for(int i=0;i<MAX_PLAYER;i++){
	    if(b->clients[0]){
	        close(b->clients[0]->sock);
	    }
	}
}

void *connection_handler(void *socket_desc){
    //Get the socket descriptor
    thread_init init= (thread_init)socket_desc;
    gameState b =  init->b;
    client cli= init->client;
    sem_wait(&b->sem_server);   //Server sem wait
    //to wait for the accept thread to complete the loop iteration
    sem_post(&b->sem_server);   //Sem Post
    free(socket_desc);

    clientMessage climen=malloc(sizeof(struct Client_Message));
    int read_size=recv(cli->sock , climen , sizeof(struct Client_Message) , 0);
    if(read_size==sizeof(struct Client_Message)) {
        if (climen->str_play1[MAX_CHAR - 1] == '\0') {
            sem_wait(&b->sem_server);
            strcpy(cli->cliName, climen->str_play1);
            sem_post(&b->sem_server);
        }
        givePlayerBoardstate(b, cli);//gives board state to player


        read_size = recv(cli->sock, climen, sizeof(struct Client_Message), 0);
        //Receive a message from client
        while (read_size > 0) {
            if (read_size == sizeof(struct Client_Message)) {
                //printCliMessage(climen);
                sem_wait(&b->sem_server);
                int gameStart = b->start;
                sem_post(&b->sem_server);
                if (gameStart > 0) {
                    switch (climen->code) {
                        case 0:
                            if (climen->x >= 0 && climen->x < BOARD_SIZE
                                && climen->y >= 0 && climen->y < BOARD_SIZE) {
                                sem_wait(&b->sem_board);
                                play_response pr = board_play(b->board, climen->x, climen->y, cli->cliNmbr);
                                print_Board(b->board);
                                sem_post(&b->sem_board);

                                switch (pr->code) {
                                    case 0://filled
                                        if (cli->state ==0) {
                                            sendErrorCode(cli, 1);
                                        } else {
                                            cli->state = 0;
                                            sem_wait(&b->sem_board);
                                            int *pointer = removeChoice(b->board, cli->cliNmbr);
                                            if (pointer != NULL) {
                                                sendOneSquareTurn(b, pointer[0], pointer[1], cli->cliColour);
                                                free(pointer);
                                                print_Board(b->board);
                                            }
                                            sem_post(&b->sem_board);
                                        }
                                        break;
                                    case 1://first play
                                        sendOneSquareTurn(b, climen->x, climen->y, cli->cliColour);
                                        int pthreadRet;
                                        cli->state = 1;
                                        pthread_t threadID;
                                        timerThreadParam param = malloc(sizeof(struct TimerParam));
                                        param->b = b;
                                        param->cl = cli;
                                        do {
                                            pthreadRet = pthread_create(&threadID, NULL, &clientPickTimer,
                                                                        (void *) param);
                                        } while (pthreadRet < 0);
                                        break;
                                    case 3:
                                    case 2://Case miss second and hit second call the same funcition

                                        cli->state = 0;
                                        secondSquareUpdate(b, pr->play1[0], pr->play1[1], pr->play2[0], pr->play2[1],
                                                           cli->cliColour);

                                        if (pr->code == 3)
                                            setNewGameStart(b);
                                        cli->timeout++;
                                        break;
                                    case -2:
                                        cli->timeout++;
                                        cli->state = 2;
                                        secondSquareUpdate(b, pr->play1[0], pr->play1[1], pr->play2[0], pr->play2[1],
                                                           cli->cliColour);
                                        pthread_t threadIDWrongChoice;
                                        timerThreadParam parameters = malloc(sizeof(struct TimerParam));
                                        parameters->b = b;
                                        parameters->cl = cli;
                                        do {
                                            pthreadRet = pthread_create(&threadIDWrongChoice, NULL, &clientWrongTimer,
                                                                        (void *) parameters);
                                        } while (pthreadRet < 0);
                                        break;
                                    case -3:
                                        sendErrorCode(cli, 6);
                                        break;
                                    default:
                                        break;
                                }
                                freePlayResponse(pr);
                            }
                            break;
                        case 1:
                            if (climen->str_play1[MAX_CHAR - 1] == '\0') {
                                sem_wait(&b->sem_server);
                                strcpy(cli->cliName, climen->str_play1);
                                sem_post(&b->sem_server);
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    sendErrorCode(cli, 2);
                }
            }
            read_size = recv(cli->sock, climen, sizeof(struct Client_Message), 0);
        }
    }
    {//client is dead
        if(read_size==0)
            printf("Client %s disconnected because of timeout",cli->cliName);
        if(read_size==-1)
            perror("recv failed");
        if(read_size>0)
            printf("The bugger is sending unknown information");
        killClient(b,cli);
    }
    return 0;
}
void killClient(gameState b, client cli){
    sem_wait(&b->sem_board);   //Server sem wait

    close(cli->sock);

    sem_wait(&b->sem_server);

    b->clients[cli->cliNmbr]=NULL;
    b->playerNumber--;
    if (b->playerNumber<2){
        b->start=0;
        sendGameState(b,5);
    }
    sem_post(&b->sem_server);
    int removalNumber=0;

    int * toRemove=removePlayer(b->board,cli->cliNmbr,&removalNumber);   //remove player taken cards (reflip them over)
    if(toRemove!=NULL){
        for(int i=0;i<removalNumber;i++)
            sendOneSquareTurn(b, toRemove[i * 2], toRemove[i * 2 + 1], cli->cliColour);
    }
    print_Board(b->board);
    free(toRemove);
    free(cli);
    sem_post(&b->sem_board);   //Server sem wait
}
void * clientPickTimer(void *param){
    if(param==NULL)
        return NULL;
    timerThreadParam tim = (timerThreadParam) param;
    client cl=tim->cl;
    gameState b= tim->b;
    int oldTimeout= ((client)cl)->timeout,* pointer=NULL;
    sleep(5);
    if(((client)cl)->timeout==oldTimeout && ((client)cl)->state==1) {
        pointer=removeChoice(b->board,((client)cl)->cliNmbr);
        if(pointer!=NULL){
            sendOneSquareTurn(b, pointer[0],pointer[1],((client)cl)->cliColour);
            free(pointer);
            print_Board(b->board);
        }
    }
    free(param);
    return NULL;
}
void * clientWrongTimer(void *param){
    if(param==NULL)
        return NULL;
    timerThreadParam tim = (timerThreadParam) param;
    client cl=tim->cl;
    gameState b= tim->b;
    sleep(2);
    if(((client)cl)->state==2) {
            struct RGBCOLOR white;
            colourSet(&white,255,255,255);
            sem_wait(&b->sem_board);
            if(isLocked(b->board,cl->cliNmbr)) {
                int plays;
                int *nPlays=&plays,* coordinates =getPlays(b->board,cl->cliNmbr,nPlays);
                if(coordinates!=NULL){
                    unlockSquare(b->board, coordinates[0], coordinates[1], coordinates[2], coordinates[3]);
                    secondSquareUpdate(b, coordinates[0], coordinates[1], coordinates[2], coordinates[3], &white);
                    print_Board(b->board);
                    sem_wait(&b->sem_server);
                    cl->state=0;
                    sem_post(&b->sem_server);
                }
            }
            else
                printf("\nWHO THE HELL IS UNLOCKING STUFF?\n\n");

            sem_post(&b->sem_board);
    }
    free(param);
    return NULL;
}
