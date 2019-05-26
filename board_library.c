#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gameConnection.h"




int linear_conv(boardLibrary b,int i, int j){
  return j*b->dim_board+i;
}
char * get_board_place_str(boardLibrary b,int i, int j){
  return b->board[linear_conv(b,i, j)].v;
}
void setBoardPlaceState(boardLibrary b, int i, int j, int state, int player){
    if (state==0){
        b->board[linear_conv(b,i,j)].player=-1;
        b->board[linear_conv(b,i,j)].state=0;
    }
    if(state==1 || state==2 || state == 3){
        b->board[linear_conv(b,i,j)].state=state;
        b->board[linear_conv(b,i,j)].player=player;
    }
}
int getBoardPlaceState(boardLibrary b, int i, int j){
    return b->board[linear_conv(b,i,j)].state;
}

int getBoardPlacePlayer(boardLibrary b, int i, int j){
    return b->board[linear_conv(b,i,j)].player;
}

int * getBoardState(boardLibrary b, int * choices){
    *choices=0;
    int * toReturn;
    int localReturn[b->dim_board*b->dim_board*2];
    for(int j=0;j<b->dim_board;j++){
        for(int i=0;i<b->dim_board;i++){
            if(getBoardPlaceState(b, i, j)!=0){
                localReturn[*choices]=i;
                localReturn[*choices+1]=j;
                *choices+=2;
            }
        }
    }
    if(*choices!=0){
        toReturn=malloc(sizeof(int)*(*choices));
        for(int i=0;i<*choices;i++)
            toReturn[i]=localReturn[i];
        *choices/=2;
        return toReturn;
    }
    return NULL;
}

int * removePlayer(boardLibrary b,int player, int * choices){
    *choices=0;
    int * toReturn;
    int localReturn[b->dim_board*b->dim_board*2];
    for(int j=0;j<b->dim_board;j++){
        for(int i=0;i<b->dim_board;i++){
            if(getBoardPlaceState(b, i, j)!=0 && getBoardPlacePlayer(b, i, j)==player){
                setBoardPlaceState(b, i, j, 0, 0);
                localReturn[*choices]=i;
                localReturn[*choices+1]=j;
                *choices+=2;
            }
        }
    }
    if(*choices!=0){
        toReturn=malloc(sizeof(int)*(*choices));
        for(int i=0;i<*choices;i++)
            toReturn[i]=localReturn[i];
        *choices/=2;
        return toReturn;
    }
    b->play1[player][0]=-1;
    b->play1[player][1]=-1;
    b->lock[player]=0;
    if(*choices%2==0)
        b->n_corrects-=*choices;
    else
        b->n_corrects-=*choices+1;
    return NULL;
}

int * removeChoice(boardLibrary b,int player){
    for(int j=0;j<b->dim_board;j++)
        for(int i=0;i<b->dim_board;i++)
            if(getBoardPlaceState(b, i, j)==1 && getBoardPlacePlayer(b, i, j)==player){
                setBoardPlaceState(b, i, j, 0, 0);
                int * toReturn=malloc(sizeof(int)*2);
                toReturn[0]=i;
                toReturn[1]=j;
                b->play1[player][0]=-1;
                b->play1[player][1]=-1;
                return toReturn;
            }
    return NULL;
}
void unlockSquare(boardLibrary b,int i1,int j1,int i2,int j2){
    int bs1= getBoardPlaceState(b, i1, j1);
    int bs2= getBoardPlaceState(b, i2, j2);
    int bp1= getBoardPlacePlayer(b, i1, j1);
    int bp2= getBoardPlacePlayer(b, i2, j2);
    if(getBoardPlacePlayer(b, i1, j1)== getBoardPlacePlayer(b, i2, j2) &&
            getBoardPlaceState(b, i1, j1)== getBoardPlaceState(b, i2, j2) && getBoardPlaceState(b, i1, j1)==3){
        b->play1[getBoardPlacePlayer(b, i1, j1)][0]=-1;
        b->play1[getBoardPlacePlayer(b, i1, j1)][1]=-1;
        b->lock[getBoardPlacePlayer(b, i1, j1)]=0;
        setBoardPlaceState(b, i1, j1, 0, 0);
        setBoardPlaceState(b, i2, j2, 0, 0);
    }
}
int isLocked(boardLibrary b, int player){
    return b->lock[player];
}
int * getPlays(boardLibrary b, int player,int * n_plays){
    if(b->lock[player]!=1)
        return NULL;
    int * to_return=malloc(sizeof(int)*4);
    if(to_return==NULL)
        return NULL;

    *n_plays=0;
    for(int j=0;j<b->dim_board;j++)
        for(int i=0;i<b->dim_board;i++)
            if(getBoardPlaceState(b, i, j)==3 && getBoardPlacePlayer(b, i, j)==player){
                int * toReturn=malloc(sizeof(int)*2);
                to_return[*n_plays]=i;
                to_return[*n_plays+1]=j;
                *n_plays+=2;
                if(*n_plays==4)
                    return to_return;
            }
    return NULL;

}
boardLibrary init_board(int dim){
  int count  = 0;
  int i, j;
  char * str_place;
  boardLibrary b=malloc(sizeof(struct Board_Library));

  b->dim_board= dim;
  b->n_corrects = 0;
  for(i=0;i<MAX_PLAYER;i++){
    b->play1[i][0]= -1;
    b->lock[i]=0;
  }
  b->board = malloc(sizeof(struct Board_Place)* dim *dim);

  for( i=0; i < (b->dim_board*b->dim_board); i++){
      b->board[i].v[0] = '\0';
  }

  for (char c1 = 'a' ; c1 < ('a'+b->dim_board); c1++){
    for (char c2 = 'a' ; c2 < ('a'+b->dim_board); c2++){
      do{
        i = random()% b->dim_board;
        j = random()% b->dim_board;
        str_place = get_board_place_str(b,i, j);
       // printf("%d %d -%s-\n", i, j, str_place);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
        setBoardPlaceState(b, i, j, 0, 0);

      do{
        i = random()% b->dim_board;
        j = random()% b->dim_board;
        str_place = get_board_place_str(b,i, j);
        //printf("%d %d -%s-\n", i, j, str_place);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
        setBoardPlaceState(b, i, j, 0, 0);
      count += 2;
      if (count == b->dim_board*b->dim_board)
        return b;
    }
  }
}

void freePlayResponse(play_response pr){
    free(pr);
}
void print_Board(boardLibrary b){
    //--->x
    //|
    //|
    //Y
    printf("---------------------\n");
    for(int j =0;j<b->dim_board;j++){//I e J transpostos para ser igual ao hud
        printf("|");
        for(int i=0;i<b->dim_board;i++){
            printf("%c%c|",b->board[linear_conv(b,i,j)].v[0],b->board[linear_conv(b,i,j)].v[1]);//I e J transpostos para ser igual ao hud
        }
        printf("\n");
        printf("---------------------\n");
    }

    printf("VISIVEL\n\n---------------------\n");
    for(int j =0;j<b->dim_board;j++){
        printf("|");
        for(int i=0;i<b->dim_board;i++){ //I e J transpostos para ser igual ao hud
            if(getBoardPlaceState(b, i, j)==0)
                printf("--|");
            if(getBoardPlaceState(b, i, j)==1)
                printf("%c%c|",b->board[linear_conv(b,i,j)].v[0],b->board[linear_conv(b,i,j)].v[1]);
            if(getBoardPlaceState(b, i, j)==2)
                printf("%c%c|",toupper(b->board[linear_conv(b,i,j)].v[0]),toupper(b->board[linear_conv(b,i,j)].v[1]));
        }
        printf("\n");
        printf("---------------------\n");
    }
}
play_response board_play(boardLibrary b,int x, int y,int playernumber){
  play_response resp=malloc(sizeof(struct pr));
  resp->code =10;

  if(strcmp(get_board_place_str(b,x, y), "")==0 || getBoardPlaceState(b, x, y)!=0
            ){
    printf("FILLED by someone else\n");
    resp->code =0;
  }
  else {
      if (b->lock[playernumber] == 1) {
          printf("Locked,sorry");
          resp->code = -3;
      }
      else {
          if (b->play1[playernumber][0] == -1) {
              printf("FIRST Play");
              resp->code = 1;
              b->play1[playernumber][0] = x;
              b->play1[playernumber][1] = y;
              resp->play1[0] = b->play1[playernumber][0];
              resp->play1[1] = b->play1[playernumber][1];
              strcpy(resp->str_play1, get_board_place_str(b, x, y));
              setBoardPlaceState(b, x, y, 1, playernumber);
          } else {
              char *first_str = get_board_place_str(b, b->play1[playernumber][0], b->play1[playernumber][1]);
              char *secnd_str = get_board_place_str(b, x, y);

              resp->play1[0] = b->play1[playernumber][0];
              resp->play1[1] = b->play1[playernumber][1];
              strcpy(resp->str_play1, first_str);
              resp->play2[0] = x;
              resp->play2[1] = y;
              strcpy(resp->str_play2, secnd_str);

              if (strcmp(first_str, secnd_str) == 0) {//CORRECT
                  printf("CORRECT!!!\n");
                  setBoardPlaceState(b, resp->play1[0], resp->play1[1], 2, playernumber);
                  setBoardPlaceState(b, x, y, 2, playernumber);
                  b->n_corrects += 2;
                  if (b->n_corrects == b->dim_board * b->dim_board)
                      resp->code = 3;
                  else
                      resp->code = 2;
              } else {                             //Incorrect
                  printf("INCORRECT");
                  setBoardPlaceState(b, resp->play1[0], resp->play1[1], 3, playernumber);
                  setBoardPlaceState(b, x, y, 3, playernumber);
                  b->lock[playernumber] = 1;
                  resp->code = -2;
              }
              b->play1[playernumber][0] = -1;
          }
      }
  }
  return resp;
}


