//
// Created by fizka on 26-05-2019.
//

//
// Created by fizka on 22-05-2019.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <semaphore.h>
#include <string.h>
#include "gameConnection.h"

void * recvThread(void * param);

void error(char *msg)
{
    printf(msg);
    exit(0);
}
typedef struct Board_Spot{
    char v[3];
    int state;
    int player;
} * boardSpot;

typedef struct Double_place{
    boardSpot first,second;
} * doubleSpot;
typedef struct RecvThreadParam{
    int sock;
    doubleSpot * pairs;
    boardSpot * board;
    sem_t * sem;
} * recvThreadParam;

int  sendFirstMessage(int sock,char * string){
    clientMessage mess= malloc(sizeof(struct Client_Message));
    strcpy(mess->str_play1,string);
    mess->str_play1[MAX_CHAR-1]='\0';
    write(sock,mess,sizeof(struct Client_Message));//Send player name
    free(mess);

    serverMessage resp= malloc(sizeof(struct Server_Message));
    int recvVal=recv(sock,resp, sizeof(struct Server_Message),0);//recieve board size
    if(recvVal!= sizeof(struct Server_Message) ||resp->code!=7) {
        free(resp);
        return -1;
    }
    int i= resp->newValue;
    free(resp);
    return i;
}
int main(int argc, char *argv[]){
    if (argc < 3) {//2
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    if(strlen(argv[2])> MAX_CHAR)
        perror("Player Name is too big");

    //Init sock
    int sockfdWinner,sockCuriosity, portno, n=1;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = PORT;//atoi(argv[2]);
    sockfdWinner = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdWinner < 0)
        error("ERROR opening socket Winner");
    sockCuriosity = socket(AF_INET, SOCK_STREAM, 0);
    if (sockCuriosity < 0)
        error("ERROR opening socket Discovery");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        error("");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfdWinner,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    if (connect(sockCuriosity,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    int size=sendFirstMessage(sockfdWinner,argv[2]);
    if(size==-1)
        error("Recived bad first message, exiting");
    size=sendFirstMessage(sockCuriosity,argv[2]);
    if(size==-1)
        error("Recived bad first message, exiting");

    boardSpot  * board = malloc(sizeof(struct Board_Spot)*size*size);
    doubleSpot * doubles = malloc(sizeof(struct Double_place)*size*size/2);

    for(int i=0;i<size*size;i++){
        if(i<size*size/2){
            doubles[i]->first=NULL;//TODO finish bot, find out why these are not pointers
            doubles[i]->second=NULL;
        }
        board[i]->state=-1;
        board[i]->player=-1;
        board[i]->
    }
    sem_t semaphore;
    if (sem_init(&semaphore, 0, 1) == -1){
        perror("sem_init error");
        exit(-1);
    }

    clientMessage cliMen=malloc(sizeof(struct Client_Message));
    if(cliMen==NULL){
        error("Error initializing client message");
    }
    serverMessage serMen=malloc(sizeof(struct Server_Message));
    if(serMen==NULL)
        error("Error initializing Server message");
    int  pthreaderr;
    pthread_t curiosityThread;
    recvThreadParam threadParam=malloc(sizeof(struct RecvThreadParam));
    threadParam->sock=sockCuriosity;
    threadParam->board=
    do{
        pthreaderr=pthread_create( &curiosityThread , NULL ,  recvThread , (void*) threadParam);
        if(pthreaderr<0)
            perror("could not create thread");
    }while (pthreaderr<0);

    int recvVal=recv(sockfd,serMen, sizeof(struct Server_Message),0);

    while(recvVal>0){

        printServerMessage(serMen);
        switch (serMen->code) {
            case 0:
                //paint_card(serMen->x, serMen->y , serMen->colour.r, serMen->colour.g, serMen->colour.b);
                if(serMen->newValue==2)//TODO change text colour, red to miss, grey on first pick and black on lock
                    // write_card(serMen->x, serMen->y, serMen->Card, 0, 0, 0);

                break;
            case 1:    //printf("Recieved card already flipped\n");// card already flipped
                break;
            case 2:
                //printf("Recieved game not started\n");// game not started
                break;
            case 3:
                //printf("Recieved game START\n");// game Start
                break;
            case 4:
                //printf("Recieved game END\n");
                //*parameters->done= 1;
                break;
            case 5:
                //printf("Recieved game PAUSE\n");    // game paused
                break;
        }
        recvVal=recv(sockfd,serMen, sizeof(struct Server_Message),0);
    }


    printf("fim\n");
}
void * recvThread(void * param){

    if(param==NULL)
        error("Error on recvThread parameter");

    recvThreadParam parameters=(recvThreadParam) param;
    int sockfd=parameters->sock;

}
