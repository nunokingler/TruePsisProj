
#ifndef BOARD_LIBRARYDEF
#define BOARD_LIBRARYDEF
#include <stdlib.h>

#define  MAX_PLAYER 10

typedef struct Board_Place{
  char v[3];
  int state;
  int player;
} board_place;

typedef struct Board_Library{
    int dim_board;
    board_place * board;
    int play1[MAX_PLAYER][2];//TODO check into making this a structure so that a server can run more than 1 game
    int lock[MAX_PLAYER];
    int n_corrects;
} * boardLibrary;

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

int * removePlayer(boardLibrary b,int player,int * Choices);//MUST CALL FREE
int * removeChoice(boardLibrary b,int player);//MUST CALL FREE
void unlockSquare(boardLibrary b,int i1,int j1,int i2,int j2);
int isLocked(boardLibrary b, int player);
int * getPlays(boardLibrary b, int player, int * n_plays);
char * get_board_place_str(boardLibrary b,int i, int j);
boardLibrary init_board(int dim);
void print_Board(boardLibrary b);
play_response board_play (boardLibrary b,int x, int y,int player);
void freePlayResponse(play_response pr);
int getBoardState(boardLibrary b,int i, int j);

#endif