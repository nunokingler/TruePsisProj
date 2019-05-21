//
// Created by fizka on 20-05-2019.
//
#include <stdio.h>
#include "gameConnection.h"
void printCliMessage(clientMessage pMessage){
    printf("ID=%d\n,x=%d\ny=%d\nString=%s\n",pMessage->code,pMessage->x,pMessage->y,pMessage->str_play1);
}
void printServerMessage(serverMessage pMessage){
    printf("ID=%d\n,card=|%s|\nNew Value=%d\nWinner=%s\nx=%d,\ny=%d\nRGB-%d-%d-%d"
            ,pMessage->code,pMessage->Card,pMessage->newValue,pMessage->winner,pMessage->x,pMessage->y
            ,pMessage->colour.r,pMessage->colour.g,pMessage->colour.b);
}