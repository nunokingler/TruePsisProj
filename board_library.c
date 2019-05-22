#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gameConnection.h"

int dim_board;
board_place * board;
int play1[MAX_PLAYER][2];//TODO check into making this a structure so that a server can run more than 1 game
int lock[MAX_PLAYER];
int n_corrects;

int linear_conv(int i, int j){
  return j*dim_board+i;
}
char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}
void setBoardState(int i, int j, int state,int player){
    if (state==0){
        board[linear_conv(i,j)].player=0;
        board[linear_conv(i,j)].state=0;
    }
    if(state==1 || state==2 || state == 3){
        board[linear_conv(i,j)].state=state;
        board[linear_conv(i,j)].player=player;
    }
}
int getBoardState(int i, int j){
    return board[linear_conv(i,j)].state;
}
int getBoardPlayer(int i,int j){
    return board[linear_conv(i,j)].player;
}
int * removePlayer(int player, int * choices){
    *choices=0;
    int * toReturn;
    int localReturn[dim_board*dim_board*2];
    for(int j=0;j<dim_board;j++){
        for(int i=0;i<dim_board;i++){
            if(getBoardState(i,j)!=0 && getBoardPlayer(i,j)==player){//TODO check if we remove pairs or only single cards
                setBoardState(i,j,0,0);
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
    play1[player][0]=-1;
    play1[player][1]=-1;
    if(*choices%2==0)
        n_corrects-=*choices;
    else
        n_corrects-=*choices+1;
    return NULL;
}

int * removeChoice(int player){
    for(int j=0;j<dim_board;j++)
        for(int i=0;i<dim_board;i++)
            if(getBoardState(i,j)==1 && getBoardPlayer(i,j)==player){
                setBoardState(i,j,0,0);
                int * toReturn=malloc(sizeof(int)*2);
                toReturn[0]=i;
                toReturn[1]=j;
                play1[player][0]=-1;
                play1[player][1]=-1;
                return toReturn;
            }
    return NULL;
}
void unlockSquare(int i1,int j1,int i2,int j2){
    int cmp=strcmp(get_board_place_str(i1,j1),get_board_place_str(i2,j2));
    int bs1= getBoardState(i1,j1);
    int bs2= getBoardState(i2,j2);
    if(getBoardPlayer(i1,j1)==getBoardPlayer(i2,j2)&&
        getBoardState(i1,j1)==getBoardState(i2,j2) && getBoardState(i1,j1)==3){
        play1[getBoardPlayer(i1,j1)][0]=-1;
        play1[getBoardPlayer(i1,j1)][1]=-1;
        lock[getBoardPlayer(i1,j1)]=0;
        setBoardState(i1,j1,0,0);
        setBoardState(i2,j2,0,0);
    }
}
board_place * init_board(int dim){
  int count  = 0;
  int i, j;
  char * str_place;

  dim_board= dim;
  n_corrects = 0;
  for(i=0;i<MAX_PLAYER;i++){
    play1[i][0]= -1;
    lock[i]=0;
  }
  board = malloc(sizeof(board_place)* dim *dim);

  for( i=0; i < (dim_board*dim_board); i++){
    board[i].v[0] = '\0';
  }

  for (char c1 = 'a' ; c1 < ('a'+dim_board); c1++){
    for (char c2 = 'a' ; c2 < ('a'+dim_board); c2++){
      do{
        i = random()% dim_board;
        j = random()% dim_board;
        str_place = get_board_place_str(i, j);
       // printf("%d %d -%s-\n", i, j, str_place);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
      setBoardState(i,j,0,0);

      do{
        i = random()% dim_board;
        j = random()% dim_board;
        str_place = get_board_place_str(i, j);
        printf("%d %d -%s-\n", i, j, str_place);
      }while(str_place[0] != '\0');
      str_place[0] = c1;
      str_place[1] = c2;
      str_place[2] = '\0';
      setBoardState(i,j,0,0);
      count += 2;
      if (count == dim_board*dim_board)
        return board;
    }
  }
}

void freePlayResponse(play_response pr){
    free(pr);
}
void print_Board(){
    //--->x
    //|
    //|
    //Y
    printf("---------------------\n");
    for(int j =0;j<dim_board;j++){//I e J transpostos para ser igual ao hud
        printf("|");
        for(int i=0;i<dim_board;i++){
            printf("%c%c|",board[linear_conv(i,j)].v[0],board[linear_conv(i,j)].v[1]);//I e J transpostos para ser igual ao hud
        }
        printf("\n");
        printf("---------------------\n");
    }

    printf("VISIVEL\n\n---------------------\n");
    for(int j =0;j<dim_board;j++){
        printf("|");
        for(int i=0;i<dim_board;i++){ //I e J transpostos para ser igual ao hud
            if(getBoardState(i,j)==0)
                printf("--|");
            if(getBoardState(i,j)==1)
                printf("%c%c|",board[linear_conv(i,j)].v[0],board[linear_conv(i,j)].v[1]);
            if(getBoardState(i,j)==2)
                printf("%c%c|",toupper(board[linear_conv(i,j)].v[0]),toupper(board[linear_conv(i,j)].v[1]));
        }
        printf("\n");
        printf("---------------------\n");
    }
}
play_response board_play(int x, int y,int playernumber){
  play_response resp=malloc(sizeof(struct pr));
  resp->code =10;

  if(strcmp(get_board_place_str(x, y), "")==0 || getBoardState(x,y)!=0
            || lock[playernumber]==1
            ){
    printf("FILLED by someone else or your locked\n");
    resp->code =0;
  }
  else{
    if(play1[playernumber][0]== -1){
        printf("FIRST Play");
        resp->code =1;
        play1[playernumber][0]=x;
        play1[playernumber][1]=y;
        resp->play1[0]= play1[playernumber][0];
        resp->play1[1]= play1[playernumber][1];
        strcpy(resp->str_play1, get_board_place_str(x, y));
        setBoardState(x,y,1,playernumber);
    }

    else{
          char * first_str = get_board_place_str(play1[playernumber][0], play1[playernumber][1]);
          char * secnd_str = get_board_place_str(x, y);

          resp->play1[0]= play1[playernumber][0];
          resp->play1[1]= play1[playernumber][1];
          strcpy(resp->str_play1, first_str);
          resp->play2[0]= x;
          resp->play2[1]= y;
          strcpy(resp->str_play2, secnd_str);

          if (strcmp(first_str, secnd_str) == 0){//CORRECT
            printf("CORRECT!!!\n");
            setBoardState(resp->play1[0],resp->play1[1],2,playernumber);
            setBoardState(x,y,2,playernumber);
            n_corrects +=2;
            if (n_corrects == dim_board* dim_board)
                resp->code = 3;
            else
              resp->code = 2;
          }
          else{                             //Incorrect
            printf("INCORRECT");
            setBoardState(resp->play1[0],resp->play1[1],3,playernumber);
            setBoardState(x,y,3,playernumber);
            lock[playernumber]=1;
            resp->code = -2;
          }
          play1[playernumber][0]= -1;
      }
    }
  return resp;
}


