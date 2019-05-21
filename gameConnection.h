//
// Created by fizka on 20-05-2019.
//

#ifndef TESTEC_GAMECONNECTION_H
#define TESTEC_GAMECONNECTION_H


#define  MAX_PLAYER 10
#define  MAX_CHAR 20
#define PORT 12346


typedef struct Server_Message{
    int code; // 0 - New card flipped
    // 1 - guess ack
    // 2 2nd - Wrong guess
    // 3 END
    int Card[2];
    int x,y;//position
    int r,g,b;//color
    char winner[MAX_CHAR];
} * serverMessage;

typedef struct Client_Message{
    int code; // 0 - guess
    //1 - set Name
    int x,y;
    char str_play1[MAX_CHAR];
} * clientMessage;

#endif //TESTEC_GAMECONNECTION_H
