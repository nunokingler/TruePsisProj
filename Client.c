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
int main(int argc, char *argv[]){
    if (argc < 3) {//2
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
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
        error("ERROR connecting");

    create_board_window(300, 300, BOARD_SIZE);
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
    do{
        pthreaderr=pthread_create( &recvThreadID , NULL ,  recvThread , (void*) threadParam);
        if(pthreaderr<0)
            error("could not create thread");
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
        sem_wait(parameters->sem);
        switch (serMen->code) {
            case 0:
                    paint_card(serMen->x, serMen->y , serMen->colour.r, serMen->colour.g, serMen->colour.b);
                    write_card(serMen->x, serMen->y, serMen->Card, 255, 255, 255);//TODO change colour of text?
                    printServerMessage(serMen);
                break;
            case 1:    printf("Recieved card already flipped\n");// card already flipped
                break;
            case 2:
                printf("Recieved game not started\n");// game not started
                break;
            case 3:
                printf("Recieved game START\n");// game Start
                paintAllCards(255,255,255,parameters->size);
                break;
            case 4://TODO game ended
                printf("Recieved game END\n");
                paintAllCards(0,0,0,parameters->size);
            //*parameters->done= 1;
                break;
            case 5:
                paintAllCards(180,180,180,parameters->size);
                printf("Recieved game PAUSE\n");    // game paused
                break;
        }
        sem_post(parameters->sem);
        recvVal=recv(sockfd,serMen, sizeof(struct Server_Message),0);
    }
    error("RECIVE ENDED");
}
