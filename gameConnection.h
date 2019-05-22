//
// Created by fizka on 20-05-2019.
//

#ifndef TESTEC_GAMECONNECTION_H
#define TESTEC_GAMECONNECTION_H


#define BOARD_SIZE 8
#define  MAX_PLAYER 10
#define  MAX_CHAR 20
#define PORT 12346

typedef struct RGBCOLOR {
    int r,g,b;
} * colour;

typedef struct Server_Message{
    int code; // 0 - New card flipped
            //  1- card already flipped
            //  2- Game not started yet
            //  3 - game Start
            //  4 - game end
            // 5 - game stop
    char Card[3];
    int newValue;
    int x,y;//position
    struct RGBCOLOR colour;
    char winner[MAX_CHAR];
} * serverMessage;

typedef struct Client_Message{
    int code; // 0 - guess
    //1 - set Name
    int x,y;
    char str_play1[MAX_CHAR];
} * clientMessage;

void printCliMessage(clientMessage pMessage);

void printServerMessage(serverMessage pMessage);


void colourCopy(colour dest,colour origin);

void colourSet(colour c,int r, int g, int b);

#endif //TESTEC_GAMECONNECTION_H
