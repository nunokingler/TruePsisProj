#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gameConnection.h"

int dim_board;
board_place * board;
int play1[MAX_PLAYER][2];
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
    if(state==1 || state==2){
        board[linear_conv(i,j)].state=state;
        board[linear_conv(i,j)].player=player;
    }
}
int getBoardState(int i, int j){
    return board[linear_conv(i,j)].state;
}
board_place * init_board(int dim){
  int count  = 0;
  int i, j;
  char * str_place;

  dim_board= dim;
  n_corrects = 0;
  for(int i=0;i<MAX_PLAYER;i++)
    play1[i][0]= -1;
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
        printf("%d %d -%s-\n", i, j, str_place);
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

  if(strcmp(get_board_place_str(x, y), "")==0 || board[linear_conv(x,y)].state!=0
            //||(play1[playernumber][0]!=-1 && (play1[playernumber][0]==x && play1[playernumber][1]==y))
            ){
    printf("FILLED by someone else or you\n");
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
        setBoardState(x,y,1,0);
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
            //strcpy(first_str, "");
            //strcpy(secnd_str, "");
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
            setBoardState(resp->play1[0],resp->play1[1],0,0);
            setBoardState(x,y,0,0);
            resp->code = -2;
          }
          play1[playernumber][0]= -1;
      }
    }
  return resp;
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
