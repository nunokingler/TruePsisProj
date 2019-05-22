
#ifndef BOARD_LIBRARYDEF
#define BOARD_LIBRARYDEF
#include <stdlib.h>

typedef struct Board_Place{
  char v[3];
  int state;
  int player;
} board_place;


typedef struct pr{
  int code; // 0 - filled
            // 1 - 1st play
            // 2 2nd - same plays
            // 3 END
            // -2 2nd - diffrent
  int play1[2];
  int play2[2];
  char str_play1[3], str_play2[3];
}* play_response;

int * removePlayer(int player,int * Choices);//MUST CALL FREE
int * removeChoice(int player);//MUST CALL FREE
void unlockSquare(int i1,int j1,int i2,int j2);
char * get_board_place_str(int i, int j);
board_place * init_board(int dim);
void print_Board();
play_response board_play (int x, int y,int player);
void freePlayResponse(play_response pr);
int getBoardState(int i, int j);

#endif