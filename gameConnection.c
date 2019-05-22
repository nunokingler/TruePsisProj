//
// Created by fizka on 20-05-2019.
//
#include <stdio.h>
#include "gameConnection.h"
void printCliMessage(clientMessage pMessage){
    printf("ID=%d\n,x=%d\ny=%d\nString=%s\n",pMessage->code,pMessage->x,pMessage->y,pMessage->str_play1);
}
void printServerMessage(serverMessage pMessage){
    printf("ID=%d,card=|%s|,New Value=%d,Winner=%s,x=%d,y=%d,RGB-%d-%d-%d\n"
            ,pMessage->code,pMessage->Card,pMessage->newValue,pMessage->winner,pMessage->x,pMessage->y
            ,pMessage->colour.r,pMessage->colour.g,pMessage->colour.b);
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