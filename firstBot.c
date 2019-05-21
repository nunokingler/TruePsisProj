#include <stdio.h>
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
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 2) {
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
    int counter=1;
    while(1){
        printf("Ready to send, press a key");
        getchar();
        clientMessage cliMessage1= malloc(sizeof(struct Client_Message));

        if(counter){
            cliMessage1->code=1;
            strcpy(cliMessage1->str_play1, "Boterino");
            cliMessage1->x=0;
            cliMessage1->y=0;
            counter=0;
        }
        else{
            cliMessage1->code=0;
            strcpy(cliMessage1->str_play1, "Boterino");
            cliMessage1->x=0;
            cliMessage1->y=0;
        }
        printf("got the first messsage \n");
        n = write(sockfd,cliMessage1, sizeof(struct Client_Message));
        if (n < 0)
             error("ERROR writing to socket");
        bzero(buffer,256);
        //n = read(sockfd,buffer,255);
        if (n < 0)
             error("ERROR reading from socket");
        printf("sent message");
    }
}
