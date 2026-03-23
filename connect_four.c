// Compile: gcc -O3 -march=native -Wall connect_four.c -o connect_four

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define RESET "\x1b[0m"

#define COLS 7
#define ROWS 6
#define MAX_MOVES (COLS * ROWS)
#define DEPTH 5

/* Overview of bitboard

  6 13 20 27 34 41 48   55 62     Additional row (never set)
+---------------------+
| 5 12 19 26 33 40 47 | 54 61     TOP mask
| 4 11 18 25 32 39 46 | 53 60
| 3 10 17 24 31 38 45 | 52 59
| 2  9 16 23 30 37 44 | 51 58
| 1  8 15 22 29 36 43 | 50 57
| 0  7 14 21 28 35 42 | 49 56 63
+---------------------+

*/
// Used to detect whether a column is full
static const uint64_t TOP = 0b1000000100000010000001000000100000010000001000000;

typedef struct {
  uint64_t
      bitboard[2];  // bitboard[0] = player 1 (X), bitboard[1] = player 2 (O)
  int height[COLS];
  int moves[MAX_MOVES];
  int counter;
} Board;

Board board_init(void) {
  Board b;
  memset(&b, 0, sizeof(b));
  for (int col = 0; col < COLS; col++) {
    b.height[col] = col * (ROWS + 1);
  }
  return b;
}

void board_print(const Board* b) {
  for (int row = ROWS - 1; row >= 0; row--) {
    for (int col = 0; col < COLS; col++) {
      int bit = col * (ROWS + 1) + row;
      if ((b->bitboard[0] >> bit) & 1) {
        printf(RED "X " RESET);
      } else if ((b->bitboard[1] >> bit) & 1) {
        printf(YELLOW "O " RESET);
      } else {
        printf(". ");
      }
    }
    printf("\n");
  }
  printf("-------------\n0 1 2 3 4 5 6\n\n");
}

void board_make_move(Board* b, int col) {
  uint64_t move = (uint64_t)1 << b->height[col]++;
  b->bitboard[b->counter & 1] ^= move;
  b->moves[b->counter++] = col;
}

void board_undo_move(Board* b) {
  int col = b->moves[--b->counter];
  uint64_t move = (uint64_t)1 << --b->height[col];
  b->bitboard[b->counter & 1] ^= move;
}

int board_is_valid(const Board* b, int col) {
  return !(TOP & ((uint64_t)1 << b->height[col]));
}

int is_win(uint64_t bitboard) {
  uint64_t bb;
  bb = bitboard & (bitboard >> 7);
  if (bb & (bb >> 14)) return 1;  // horizontal
  bb = bitboard & (bitboard >> 1);
  if (bb & (bb >> 2)) return 1;  // vertical
  bb = bitboard & (bitboard >> 6);
  if (bb & (bb >> 12)) return 1;  // diagonal '\'
  bb = bitboard & (bitboard >> 8);
  if (bb & (bb >> 16)) return 1;  // diagonal '/'
  return 0;
}

// ######### ENGINE #########

int negamax(Board* b, int depth) {
  if (is_win(b->bitboard[(b->counter - 1) & 1])) return -depth - 1;
  if (depth == 0 || b->counter == MAX_MOVES) return 0;

  int best = -(DEPTH + 1);
  for (int col = 0; col < COLS; col++) {
    if (board_is_valid(b, col)) {
      board_make_move(b, col);
      int score = -negamax(b, depth - 1);
      board_undo_move(b);
      if (score > best) best = score;
    }
  }
  return best;
}

int best_move(Board* b, int depth) {
  int best_col = -1;
  int best_score = -(DEPTH + 2);
  for (int col = 0; col < COLS; col++) {
    if (board_is_valid(b, col)) {
      board_make_move(b, col);
      int score = -negamax(b, depth - 1);
      board_undo_move(b);
      if (score > best_score) {
        best_score = score;
        best_col = col;
      }
    }
  }
  return best_col;
}

int main(void) {
  Board b = board_init();

  printf("Who goes first? (0 = You, 1 = AI): ");
  int human_player = (getchar() - '0');
  while (getchar() != '\n');
  if (human_player != 0 && human_player != 1) {
    printf("Invalid choice, defaulting to you first.\n");
    human_player = 0;
  }

  // Main game loop
  while (1) {
    board_print(&b);

    int col;
    if ((b.counter & 1) == human_player) {
      // Human's turn
      if (human_player)
        printf("Choose column (0-6) for %sO%s: ", YELLOW, RESET);
      else
        printf("Choose column (0-6) for %sX%s: ", RED, RESET);

      col = getchar() - '0';
      while (getchar() != '\n');

      if (col < 0 || col >= COLS || !board_is_valid(&b, col)) {
        printf("Invalid move!\n");
        continue;
      }
    } else {
      // AI's turn
      col = best_move(&b, DEPTH);
      printf("AI played: %d\n", col);
    }

    board_make_move(&b, col);

    // Check for win
    if (is_win(b.bitboard[(b.counter - 1) & 1])) {
      board_print(&b);
      if (((b.counter - 1) & 1) == human_player)
        printf("You win!\n");
      else
        printf("AI wins!\n");
      break;
    }

    // Check for draw
    if (b.counter == MAX_MOVES) {
      board_print(&b);
      printf("Draw!\n");
      break;
    }
  }

  return 0;
}