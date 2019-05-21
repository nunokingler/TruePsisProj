//
// Created by fizka on 20-05-2019.
//

#ifndef TESTEC_GAMECONNECTION_H
#define TESTEC_GAMECONNECTION_H

#include "board_library.h"

#define  MAX_PLAYER 10
#define  MAX_CHAR 20
#define PORT 12345


typedef struct Server_Message{
    int code; // 0 - New card flipped
            //1 - game end
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

#endif //TESTEC_GAMECONNECTION_H
