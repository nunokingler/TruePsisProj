#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string.h>
#include <stdlib.h>

#include "gameConnection.h"

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n=1;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 2) {//2
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
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
    //printf("Please enter the message: ");
    //bzero(buffer,256);
    //fgets(buffer,255,stdin);
    int x,y;
    if(fork()==0){
        while(n>0){
            printf("Ready to send, press a key:");
            scanf(" %d %d",&x,&y);
            clientMessage cliMessage1= malloc(sizeof(struct Client_Message));

                cliMessage1->code=0;
                strcpy(cliMessage1->str_play1, "Boterino");
                cliMessage1->x=x;
                cliMessage1->y=y;
            n = write(sockfd,cliMessage1, sizeof(struct Client_Message));
            if (n == 0)
                 error("Socket disconnected");
            bzero(buffer,256);
            //n = read(sockfd,buffer,255);
            if (n ==-1)
                 error("ERROR reading from socket");
        }

    }
    else{
        serverMessage serverMen = malloc(sizeof(struct Server_Message));
        int read_size=recv(sockfd , serverMen , sizeof(struct Server_Message) , 0);
        while(read_size>0){
            if(read_size== sizeof(struct Server_Message)){
                printf("New Message - ");
                printServerMessage(serverMen);
                read_size=recv(sockfd , serverMen, sizeof(struct Server_Message) , 0);
            }
            else{
                printf("SOMETHING WENT WRONG,ABORT");
            }
        }
    }
}

