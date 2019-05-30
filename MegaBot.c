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
} * boardSpot;

typedef struct Double_place{
    boardSpot first,second;
    int xFirst,xSecond,yFirst,ySecond;
} * doubleSpot;
typedef struct RecvThreadParam{
    int sock;
    doubleSpot * pairs;
    boardSpot * board;
    sem_t * sem;
    int * done;
    int size;
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
    printf("teste");
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

    int size=sendFirstMessage(sockfdWinner,"MEGABOT WINS!");
    fprintf(stdout,"\n\nsize ofs %d , %d,size %d %d",sizeof(struct Board_Spot)*size*size,sizeof(struct Double_place)*size*size/2,
            sizeof(struct Board_Spot),sizeof(struct Double_place));

    printf("\n\nsize ofs %d , %d,size %d",sizeof(struct Board_Spot)*size*size,sizeof(struct Double_place)*size*size/2,sizeof(struct Double_place));
    if(size==-1)
        error("Recived bad first message, exiting");
    size=sendFirstMessage(sockCuriosity,"MEGABOT scout");
    if(size==-1)
        error("Recived bad first message, exiting");
    boardSpot  * board = malloc(sizeof(boardSpot)*size*size);
    if(board==NULL)
        error("failed to malloc board");
    doubleSpot *  pairs = malloc(sizeof(doubleSpot)*size*size/2);
    if(pairs==NULL)
        error("failed to malloc pairs");
    printf("teste2");
    for(int i=0;i<size*size;i++){
        if(i<size*size/2){
            pairs[i]= malloc(sizeof(struct Double_place));
            pairs[i]->first=NULL;//TODO finish bot, find out why these are not pointers
            pairs[i]->second=NULL;
            pairs[i]->xFirst=-1;
            pairs[i]->yFirst=-1;
            pairs[i]->xSecond=-1;
            pairs[i]->ySecond=-1;

        }
        board[i]=malloc(sizeof(struct Board_Spot));
        board[i]->state=-1;
        strcpy(board[i]->v,"");
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
    int done=0;
    pthread_t curiosityThread;
    recvThreadParam threadParam=malloc(sizeof(struct RecvThreadParam));
    threadParam->sock=sockCuriosity;
    threadParam->board=board;
    threadParam->sem=&semaphore;
    threadParam->done=&done;
    threadParam->size=size;
    do{
        pthreaderr=pthread_create( &curiosityThread , NULL ,  recvThread , (void*) threadParam);
        if(pthreaderr<0)
            perror("could not create thread");
    }while (pthreaderr<0);

    int recvVal=recv(sockfdWinner,serMen, sizeof(struct Server_Message),0);
    int pairPosition,boardPosition,scouted;
    while(recvVal>0){

        printServerMessage(serMen);
        switch (serMen->code) {
            case 0:
                //paint_card(serMen->x, serMen->y , serMen->colour.r, serMen->colour.g, serMen->colour.b);
                scouted=0;
                //fprintf(stdout,"MEGABOT recieved info on %d %d, %s-",serMen->x,serMen->y,serMen->Card);

                sem_wait(&semaphore);

                boardPosition = serMen->x+serMen->y*size;
                board[boardPosition]->state=serMen->newValue;
               // printf("MEGABOT recieved info on %d %d, %s-",serMen->x,serMen->y,serMen->Card);
                if(serMen->newValue!=0){
                    pairPosition = serMen->Card[1]-'a' + (serMen->Card[0]-'a')*size;
                    strcpy(board[boardPosition]->v,serMen->Card);

                    if(pairs[pairPosition]->first==NULL ){//first time reciving this combination
                        pairs[pairPosition]->first=board[boardPosition];
                        pairs[pairPosition]->xFirst=serMen->x;
                        pairs[pairPosition]->yFirst=serMen->y;
                    }
                    else{
                        if(pairs[pairPosition]->second==NULL &&(pairs[pairPosition]->xFirst!=serMen->x || pairs[pairPosition]->yFirst!=serMen->y)){
                            pairs[pairPosition]->second=board[boardPosition];
                            pairs[pairPosition]->xSecond=serMen->x;
                            pairs[pairPosition]->ySecond=serMen->y;
                        }
                    }
                }
                else
                    pairPosition=board[boardPosition]->v[1]-'a' + (board[boardPosition]->v[0]-'a')*size;


                if(serMen->newValue==0 && pairs[pairPosition]->first!= NULL && pairs[pairPosition]->second!= NULL ){
                    if(pairs[pairPosition]->first->state==0 && pairs[pairPosition]->second->state==0){
                        cliMen->code=0;
                        cliMen->x=pairs[pairPosition]->xFirst;
                        cliMen->y=pairs[pairPosition]->yFirst;
                        write(sockfdWinner,cliMen,sizeof(struct Client_Message));
                        cliMen->x=pairs[pairPosition]->xSecond;
                        cliMen->y=pairs[pairPosition]->ySecond;
                        recvVal=write(sockfdWinner,cliMen,sizeof(struct Client_Message));
                    }
                }
                sem_post(&semaphore);
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
                done= 1;
                break;
            case 5:
                //printf("Recieved game PAUSE\n");    // game paused
                break;
        }
        recvVal=recv(sockfdWinner,serMen, sizeof(struct Server_Message),0);
    }


    printf("fim\n");
}
void * recvThread(void * param){

    if(param==NULL)
        error("Error on recvThread parameter");

    recvThreadParam parameters=(recvThreadParam) param;
    int sockfd=parameters->sock,* done =parameters->done,size=parameters->size;
    boardSpot * board = parameters->board;
    doubleSpot * pais=parameters->pairs;
    sem_t semaphore=*parameters->sem;
    free(param);
    int x,y;
    clientMessage climen=malloc(sizeof(struct Client_Message));
    climen->code=0;
    strcpy(climen->str_play1,"");
    while(*done==0){
        sem_wait(&semaphore);
        for(x=0,y=-1;x<size*size;x++){
                if(x%size==0 )
                    y++;
                if(//board[x%size+y*size]->state==-1)
                    strcmp(board[x]->v,"")==0)
                    break;
        }
        fprintf(stdout,"sending to %d %d \n",x,y);
        climen->x=x%size;
        climen->y=y;
        write(sockfd,climen,sizeof(struct Client_Message));
        sem_post(&semaphore);
        sleep(2);
    }
}
