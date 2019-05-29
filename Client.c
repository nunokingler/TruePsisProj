//
// Created by fizka on 22-05-2019.
//

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <semaphore.h>
#include "UI_library.h"
#include "gameConnection.h"

void * recvThread(void * param);

void error(char *msg)
{
    perror(msg);
    exit(0);
}
typedef struct RecvThreadParam{
    int sock;
    int size;
    int * done;
    sem_t * sem;
} * recvThreadParam;

void paintAllCards(int r,int g,int b,int size){
    for(int i=0;i<size;i++)
        for(int j=0;j<size;j++)
            paint_card(i,j,r,g,b);
}
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
        fprintf(stderr,"usage %s hostname \n", argv[0]);
        exit(0);
    }
    if(strlen(argv[2])> MAX_CHAR)
        perror("Player Name is too big");

    //INIT SDL
    SDL_Event event;
    int done = 0;

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        exit(-1);
    }
    if(TTF_Init()==-1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(2);
    }
    //Init sock
    int sockfd, portno, n=1;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    portno = PORT;//atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        perror("ERROR connecting");

    int size=sendFirstMessage(sockfd,argv[2]);
    if(size==-1)
        perror("Recived bad first message, exiting");

    create_board_window(300, 300, size);
    sem_t semaphore;
    if (sem_init(&semaphore, 0, 1) == -1){
        perror("sem_init error");
        exit(-1);
    }
    pthread_t recvThreadID;


    int pthreaderr;
    recvThreadParam threadParam=malloc(sizeof(struct RecvThreadParam));
    threadParam->sock=sockfd;
    threadParam->done=&done;
    threadParam->sem=&semaphore;
    threadParam->size=BOARD_SIZE;

    paintAllCards(255,0,0,BOARD_SIZE);
    do{
        pthreaderr=pthread_create( &recvThreadID , NULL ,  recvThread , (void*) threadParam);
        if(pthreaderr<0)
            perror("could not create thread");
    }while (pthreaderr<0);
        clientMessage cliMen=malloc(sizeof(struct Client_Message));
        if(cliMen==NULL){
        error("Error initializing client message");
        }
        while (!done) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT: {
                        done = SDL_TRUE;
                        break;
                    }
                    case SDL_MOUSEBUTTONDOWN: {
                        sem_wait(&semaphore);
                        int board_x, board_y;
                        get_board_card(event.button.x, event.button.y, &board_x, &board_y);
                        cliMen->code = 0;
                        cliMen->x = board_x;
                        cliMen->y = board_y;
                        strcpy(cliMen->str_play1, "");
                        //printf("click (%d %d) -> (%d %d)\n", event.button.x, event.button.y, board_x, board_y);
                        write(sockfd, cliMen, sizeof(struct Client_Message));
                        refresh_card();
                        sem_post(&semaphore);
                    }
                }
            }
        }
    printf("fim\n");
    close_board_windows();
}
void * recvThread(void * param){

    if(param==NULL)
        error("Error on recvThread parameter");

    serverMessage serMen=malloc(sizeof(struct Server_Message));
    if(serMen==NULL)
        error("Error initializing Server message");

    recvThreadParam parameters=(recvThreadParam) param;
    int sockfd=parameters->sock;
    int recvVal=recv(sockfd,serMen, sizeof(struct Server_Message),0);

    while(recvVal>0){

        printServerMessage(serMen);
        sem_wait(parameters->sem);
        switch (serMen->code) {
            case 0:
                    paint_card(serMen->x, serMen->y , serMen->colour.r, serMen->colour.g, serMen->colour.b);
                    if(serMen->newValue==2)//TODO change text colour, red to miss, grey on first pick and black on lock
                        write_card(serMen->x, serMen->y, serMen->Card, 0, 0, 0);
                    else{
                        if(serMen->newValue==3)
                        write_card(serMen->x, serMen->y, serMen->Card, 255, 0, 0);
                        else write_card(serMen->x, serMen->y, serMen->Card, 193, 193, 193);
                    }
                break;
            case 1:    //printf("Recieved card already flipped\n");// card already flipped
                break;
            case 2:
                //printf("Recieved game not started\n");// game not started
                break;
            case 3:
                //printf("Recieved game START\n");// game Start
                paintAllCards(255,255,255,parameters->size);
                break;
            case 4:
                //printf("Recieved game END\n");
                paintAllCards(0,0,0,parameters->size);
                //*parameters->done= 1;
                break;
            case 5:
                paintAllCards(180,180,180,parameters->size);//TODO pause breaks already filled squares
                //printf("Recieved game PAUSE\n");    // game paused
                break;
        }
        sem_post(parameters->sem);
        recvVal=recv(sockfd,serMen, sizeof(struct Server_Message),0);
    }
    error("RECIVE ENDED");
}
