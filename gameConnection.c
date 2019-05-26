//
// Created by fizka on 20-05-2019.
//
#include <stdio.h>
#include "gameConnection.h"
void printCliMessage(clientMessage pMessage){
    printf("ID=%d\n,x=%d\ny=%d\nString=%s\n",pMessage->code,pMessage->x,pMessage->y,pMessage->str_play1);
}
void printServerMessage(serverMessage pMessage){
    switch (pMessage->code){
        case 0:    printf("ID=%d,card=|%s|,New Value=%d,x=%d,y=%d,RGB-%d-%d-%d\n"
                    ,pMessage->code,pMessage->Card,pMessage->newValue,pMessage->x,pMessage->y
                    ,pMessage->colour.r,pMessage->colour.g,pMessage->colour.b);
        break;
        case 1:printf("Card Already flipped %d %d\n",pMessage->x,pMessage->y);
            break;
        case 2:printf("Game not started\n");
            break;
        case 3:printf("Game start\n");
            break;
        case 4:printf("Card End\n");
            break;
        case 5:printf("Card Pause\n");
            break;
        case 6:printf("You are locked\n");
            break;
        default:    printf("ID=%d,card=|%s|,New Value=%d,Winner=%s,x=%d,y=%d,RGB-%d-%d-%d\n"
                    ,pMessage->code,pMessage->Card,pMessage->newValue,pMessage->winner,pMessage->x,pMessage->y
                    ,pMessage->colour.r,pMessage->colour.g,pMessage->colour.b);break;
    }
}
void colourSet(colour c,int r, int g, int b){
    if(c==NULL)
        return;
    c->r=r;
    c->g=g;
    c->b=b;
}
void colourCopy(colour dest,colour origin){
    if(dest!=NULL && origin!=NULL){
        dest->r=origin->r;
        dest->g=origin->g;
        dest->b=origin->b;
    }
}