#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#define PAD_X 1
#define PAD_Y 1
#define PRESSED_KEYS_BUFFER_SIZE 5
#define GAME_WORD_SIZE 5
#define GAME_WORDS_COUNT 6
#define MAX_CANDIDATES 15
// #define DEBUG

typedef struct {
  uint8_t valid;
  char* pressed_key;
} input_result_t;

typedef struct {
  int code_arr[PRESSED_KEYS_BUFFER_SIZE];
  uint8_t size;
  const char* str_repr;
} input_member_t;

typedef struct {
  const char* str_repr;
  size_t position;
  uint8_t is_satisfied;  // only relevant for yellow state
  uint8_t has_been_seen_as_yellow_or_green;  // only relevant for gray state
} char_rule_t;

static const char A[] = "A";
static const char B[] = "B";
static const char C[] = "C";
static const char Ç[] = "Ç";
static const char D[] = "D";
static const char E[] = "E";
static const char F[] = "F";
static const char G[] = "G";
static const char Ğ[] = "Ğ";
static const char H[] = "H";
static const char I[] = "I";
static const char İ[] = "İ";
static const char J[] = "J";
static const char K[] = "K";
static const char L[] = "L";
static const char M[] = "M";
static const char N[] = "N";
static const char O[] = "O";
static const char Ö[] = "Ö";
static const char P[] = "P";
static const char Q[] = "Q";
static const char R[] = "R";
static const char S[] = "S";
static const char Ş[] = "Ş";
static const char T[] = "T";
static const char U[] = "U";
static const char Ü[] = "Ü";
static const char V[] = "V";
static const char W[] = "W";
static const char X[] = "X";
static const char Y[] = "Y";
static const char Z[] = "Z";
static const char SPACE[] = "Space";
static const char UP[] = "Up";
static const char DOWN[] = "Down";
static const char RIGHT[] = "Right";
static const char LEFT[] = "Left";
static const char DEL[] = "Del";
static const char BACKSPACE[] = "Backspace";
static const char ENTER[] = "Enter";
static const char EMPTY_SPACE[] = " ";
static const char CURSOR_INDICATOR[] = "^";

static int LAST_PRESSED_KEYS[PRESSED_KEYS_BUFFER_SIZE];
static int LAST_READ_BYTES[PRESSED_KEYS_BUFFER_SIZE];
static int GAME_CURSOR[] = { 0, 0 };  // word, char e.g. 2, 3 => 3rd word 4th char
static char* GAME_BOARD[GAME_WORDS_COUNT][GAME_WORD_SIZE];  // since this is static, all elements will be initialized to NULL by default
static char GAME_STATE[GAME_WORDS_COUNT][GAME_WORD_SIZE];  // since this is static, all elements will be initialized to '\0' by default
static input_member_t INPUT_MAP[] = {
  { .code_arr={ 97 }, .size=1, .str_repr=A },
  { .code_arr={ 65 }, .size=1, .str_repr=A },
  { .code_arr={ 98 }, .size=1, .str_repr=B },
  { .code_arr={ 66 }, .size=1, .str_repr=B },
  { .code_arr={ 99 }, .size=1, .str_repr=C },
  { .code_arr={ 67 }, .size=1, .str_repr=C },
  { .code_arr={ 195, 167 }, .size=2, .str_repr=Ç },
  { .code_arr={ 195, 135 }, .size=2, .str_repr=Ç },
  { .code_arr={ 100 }, .size=1, .str_repr=D },
  { .code_arr={ 68 }, .size=1, .str_repr=D },
  { .code_arr={ 101 }, .size=1, .str_repr=E },
  { .code_arr={ 69 }, .size=1, .str_repr=E },
  { .code_arr={ 102 }, .size=1, .str_repr=F },
  { .code_arr={ 70 }, .size=1, .str_repr=F },
  { .code_arr={ 103 }, .size=1, .str_repr=G },
  { .code_arr={ 71 }, .size=1, .str_repr=G },
  { .code_arr={ 196, 159 }, .size=2, .str_repr=Ğ },
  { .code_arr={ 196, 158 }, .size=2, .str_repr=Ğ },
  { .code_arr={ 104 }, .size=1, .str_repr=H },
  { .code_arr={ 72 }, .size=1, .str_repr=H },
  { .code_arr={ 196, 177 }, .size=2, .str_repr=I },
  { .code_arr={ 73 }, .size=1, .str_repr=I },
  { .code_arr={ 0xC4, 0xB1 }, .size=2, .str_repr=I },
  { .code_arr={ 105 }, .size=1, .str_repr=İ },
  { .code_arr={ 196, 176 }, .size=2, .str_repr=İ },
  { .code_arr={ 106 }, .size=1, .str_repr=J },
  { .code_arr={ 74 }, .size=1, .str_repr=J },
  { .code_arr={ 107 }, .size=1, .str_repr=K },
  { .code_arr={ 75 }, .size=1, .str_repr=K },
  { .code_arr={ 108 }, .size=1, .str_repr=L },
  { .code_arr={ 76 }, .size=1, .str_repr=L },
  { .code_arr={ 109 }, .size=1, .str_repr=M },
  { .code_arr={ 77 }, .size=1, .str_repr=M },
  { .code_arr={ 110 }, .size=1, .str_repr=N },
  { .code_arr={ 78 }, .size=1, .str_repr=N },
  { .code_arr={ 111 }, .size=1, .str_repr=O },
  { .code_arr={ 79 }, .size=1, .str_repr=O },
  { .code_arr={ 195, 182 }, .size=2, .str_repr=Ö },
  { .code_arr={ 195, 150 }, .size=2, .str_repr=Ö },
  { .code_arr={ 112 }, .size=1, .str_repr=P },
  { .code_arr={ 80 }, .size=1, .str_repr=P },
  { .code_arr={ 113 }, .size=1, .str_repr=Q },
  { .code_arr={ 81 }, .size=1, .str_repr=Q },
  { .code_arr={ 114 }, .size=1, .str_repr=R },
  { .code_arr={ 82 }, .size=1, .str_repr=R },
  { .code_arr={ 115 }, .size=1, .str_repr=S },
  { .code_arr={ 83 }, .size=1, .str_repr=S },
  { .code_arr={ 197, 159 }, .size=2, .str_repr=Ş },
  { .code_arr={ 197, 158 }, .size=2, .str_repr=Ş },
  { .code_arr={ 116 }, .size=1, .str_repr=T },
  { .code_arr={ 84 }, .size=1, .str_repr=T },
  { .code_arr={ 117 }, .size=1, .str_repr=U },
  { .code_arr={ 85 }, .size=1, .str_repr=U },
  { .code_arr={ 195, 188 }, .size=2, .str_repr=Ü },
  { .code_arr={ 195, 156 }, .size=2, .str_repr=Ü },
  { .code_arr={ 118 }, .size=1, .str_repr=V },
  { .code_arr={ 86 }, .size=1, .str_repr=V },
  { .code_arr={ 119 }, .size=1, .str_repr=W },
  { .code_arr={ 87 }, .size=1, .str_repr=W },
  { .code_arr={ 120 }, .size=1, .str_repr=X },
  { .code_arr={ 88 }, .size=1, .str_repr=X },
  { .code_arr={ 121 }, .size=1, .str_repr=Y },
  { .code_arr={ 89 }, .size=1, .str_repr=Y },
  { .code_arr={ 122 }, .size=1, .str_repr=Z },
  { .code_arr={ 90 }, .size=1, .str_repr=Z },
  { .code_arr={ 32 }, .size=1, .str_repr=SPACE },
  { .code_arr={ 27, 91, 65 }, .size=3, .str_repr=UP },
  { .code_arr={ 27, 91, 66 }, .size=3, .str_repr=DOWN },
  { .code_arr={ 27, 91, 67 }, .size=3, .str_repr=RIGHT },
  { .code_arr={ 27, 91, 68 }, .size=3, .str_repr=LEFT },
  { .code_arr={ 27, 91, 51, 126 }, .size=4, .str_repr=DEL },
  { .code_arr={ 127 }, .size=1, .str_repr=BACKSPACE },
  { .code_arr={ 10 }, .size=1, .str_repr=ENTER },
};
static char* (*WORDS)[GAME_WORD_SIZE] = NULL;
static size_t WORDS_COUNT = 0;
static size_t CANDIDATE_IDX[MAX_CANDIDATES];
static size_t FOUND_CANDIDATES = 0;

int compare_by_size_desc(const void* a, const void* b) {
  const input_member_t* result_a = (const input_member_t*)a;
  const input_member_t* result_b = (const input_member_t*)b;

  return (result_b->size - result_a->size);
}

int resolve_input_map_idx(int* buf, int count) {
  for (size_t i = 0; i < (sizeof(INPUT_MAP)/sizeof(INPUT_MAP[0])); i++) {
    for (int j = INPUT_MAP[i].size - 1; j >= 0; j--) {
      if (INPUT_MAP[i].code_arr[j] != buf[count + j - INPUT_MAP[i].size]) {
        break;
      }

      if (j == 0) return i;
    }
  }

  return -1;
}

void resolve_char(int* buf, int count, int c, input_result_t* result) {
  // for (int i = 1; i < PRESSED_KEYS_BUFFER_SIZE; i++) {
  //   LAST_PRESSED_KEYS[i - 1] = LAST_PRESSED_KEYS[i];
  // }
  memmove(buf, buf + 1, (count - 1) * sizeof(int));

  buf[count - 1] = c;

  // input_result_t* result = malloc(sizeof(input_result_t));
  result->pressed_key = NULL;
  result->valid = 0;

  int idx = resolve_input_map_idx(buf, count);

  if (idx != -1) {
    result->pressed_key = INPUT_MAP[idx].str_repr;
    result->valid = 1;
  }
}

void resolve_input(input_result_t* result) {
  // TODO: maybe instead of getchar, we should use read(STDIN_FILENO, buf, sizeof buf) because then we will get whole bytes in one call.
  // also, we should use polling to differentiate between raw keys like Esc(27) and escaped sequences like Up arrow(27, 91, 65)
  int c = getchar();

  resolve_char(LAST_PRESSED_KEYS, PRESSED_KEYS_BUFFER_SIZE, c, result);
}

void resolve_read_byte(int c, input_result_t* result) {
  resolve_char(LAST_READ_BYTES, PRESSED_KEYS_BUFFER_SIZE, c, result);
}

void set_buffered_input(int enable) {
  static int enabled = 1;
  static struct termios old;
  struct termios new;

  if (enable && !enabled) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    enabled = 1;
  } else if (!enable && enabled) {
    tcgetattr(STDIN_FILENO, &new);
    old = new;
    new.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
    enabled = 0;
  }
}

void signal_callback_handler(int signum) {
  set_buffered_input(1);
  // make cursor visible, reset all modes, restore old term buffer
  printf("\033[?25h\033[m\033[?1049l");
  exit(signum);
}

/**
  * reads the words file
  * returns 1 on success and 0 on error.
*/
int read_words_file() {
  FILE* fp = fopen("words.txt", "rb");

  if (fp == NULL) {
    printf("The words file can not be opened.\n");
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  const long file_size = ftell(fp);
  // if this is not unsigned char, then the bytes read from file becomes negative and characters does not match.
  unsigned char* read_bytes = malloc(file_size);

  if (read_bytes == NULL) {
    printf("Could not allocate memory for file read\n");
    fclose(fp);
    return 0;
  }

  // fseek(stream, 0, SEEK_SET);
  rewind(fp);
  fread(read_bytes, sizeof(char), file_size, fp);
  fclose(fp);

  WORDS_COUNT = 0;
  int c1 = 0;
  int c2 = 0;
  for (int i = 0; i < file_size; i++) {
    if (read_bytes[i] == 10) {
      c2 = i;
      if (c2 - c1 >= GAME_WORD_SIZE) {
        WORDS_COUNT++;
      }
      c1 = c2;
    }
  }

  if (read_bytes[file_size - 1] != 10) WORDS_COUNT++;

  WORDS = malloc(WORDS_COUNT * sizeof(*WORDS));
  if (WORDS == NULL) {
    printf("Could not allocate memory for words\n");
    free(read_bytes);
    return 0;
  }

  input_result_t* result = malloc(sizeof(input_result_t));
  size_t w = 0;
  size_t c = 0;
  for (int i = 0; i < file_size; i++) {
    resolve_read_byte(read_bytes[i], result);
    if (result->valid) {
      if (result->pressed_key == ENTER) {
        if (c != 0) {
          printf("Found strange word - w: %zu c: %zu\n", w, c);
          return 0;
        }
        continue;
      }

      WORDS[w][c] = result->pressed_key;
      c++;
      if (c == GAME_WORD_SIZE) {
        c = 0;
        w++;
      }
    }
  }

  free(read_bytes);
  free(result);
  return 1;
}

void run_analysis() {
  FOUND_CANDIDATES = 0;

  size_t valid_words = 0;
  char_rule_t greens[GAME_WORDS_COUNT * GAME_WORD_SIZE];
  size_t green_count = 0;
  char_rule_t yellows[GAME_WORDS_COUNT * GAME_WORD_SIZE];
  size_t yellow_count = 0;
  char_rule_t grays[GAME_WORDS_COUNT * GAME_WORD_SIZE];
  size_t gray_count = 0;
  for (int i = 0; i < GAME_WORDS_COUNT; i++) {
    uint8_t word_valid = 1;
    for (int j = 0; j < GAME_WORD_SIZE; j++) {
      if (!GAME_BOARD[i][j]) {
        word_valid = 0;
        break;
      }
    }

    if (!word_valid) break;

    valid_words++;
    for (int j = 0; j < GAME_WORD_SIZE; j++) {
      if (GAME_STATE[i][j] == 'G') {
        greens[green_count] = (char_rule_t){ .str_repr=GAME_BOARD[i][j], .position=j, .is_satisfied=0, .has_been_seen_as_yellow_or_green=0 };
        green_count++;
        continue;
      }

      if (GAME_STATE[i][j] == 'Y') {
        yellows[yellow_count] = (char_rule_t){ .str_repr=GAME_BOARD[i][j], .position=j, .is_satisfied=0, .has_been_seen_as_yellow_or_green=0 };
        yellow_count++;
        continue;
      }

      uint8_t seen_as_yellow_or_green = 0;
      for (int k = 0; k < GAME_WORD_SIZE; k++) {
        if (GAME_BOARD[i][k]==GAME_BOARD[i][j] && (GAME_STATE[i][k] == 'Y' || GAME_STATE[i][k] == 'G')) {
          seen_as_yellow_or_green = 1;
          break;
        }
      }

      grays[gray_count] = (char_rule_t){ .str_repr=GAME_BOARD[i][j], .position=j, .is_satisfied=0, .has_been_seen_as_yellow_or_green=seen_as_yellow_or_green };
      gray_count++;
    }
  }

  if (valid_words == 0) return;

  for (size_t i = 0; i < WORDS_COUNT; i++) {
    uint8_t valid_candidate = 1;

    for (size_t y = 0; y < yellow_count; y++) {
      yellows[y].is_satisfied = 0;
    }

    for (size_t j = 0; j < GAME_WORD_SIZE; j++) {
      for (size_t g = 0; g < gray_count; g++) {
        if (!grays[g].has_been_seen_as_yellow_or_green && grays[g].str_repr == WORDS[i][j]) {
          valid_candidate = 0;
          break;
        }

        if (grays[g].has_been_seen_as_yellow_or_green && grays[g].str_repr == WORDS[i][j] && grays[g].position == j) {
          valid_candidate = 0;
          break;
        }
      }

      if (!valid_candidate) break;

      for (size_t g = 0; g < green_count; g++) {
        if (greens[g].str_repr != WORDS[i][j] && greens[g].position == j) {
          valid_candidate = 0;
          break;
        }
      }

      if (!valid_candidate) break;

      for (size_t y = 0; y < yellow_count; y++) {
        if (yellows[y].str_repr == WORDS[i][j] && yellows[y].position == j) {
          valid_candidate = 0;
          break;
        }

        if (yellows[y].is_satisfied) continue;

        if (yellows[y].str_repr == WORDS[i][j]) {
          yellows[y].is_satisfied = 1;
        }
      }
    }

    for (size_t y = 0; valid_candidate && y < yellow_count; y++) {
      valid_candidate = valid_candidate && yellows[y].is_satisfied;
    }

    if (valid_candidate) {
      CANDIDATE_IDX[FOUND_CANDIDATES] = i;
      FOUND_CANDIDATES++;

      if (FOUND_CANDIDATES >= MAX_CANDIDATES) {
        break;
      }
    }
  }
}

void cycle_state() {
  if (GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] ==  0 ) { GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] = 'Y'; return; }
  if (GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] == 'Y') { GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] = 'G'; return; }
  if (GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] == 'G') { GAME_STATE[GAME_CURSOR[0]][GAME_CURSOR[1]] =  0 ; return; }
}

void draw_board() {
  // clear entire screen
  printf("\033[2J");

  for (int i = 0; i < GAME_WORDS_COUNT; i++) {
    for (int j = 0; j < GAME_WORD_SIZE; j++) {
      int bg_color = 100;  // bright black - gray
      if (GAME_STATE[i][j] == 'G') {
        bg_color = 42;  // green
      } else if (GAME_STATE[i][j] == 'Y') {
        bg_color = 43;  // yellow
      }

      int row = (i * (3 + PAD_Y)) + 1;
      int col = (j * (5 + PAD_X)) + 1;
      char* letter = EMPTY_SPACE;
      char* cursor_indicator = EMPTY_SPACE;
      if (GAME_BOARD[i][j]) {
        letter = GAME_BOARD[i][j];
      }

      if (GAME_CURSOR[0] == i && GAME_CURSOR[1] == j) {
        cursor_indicator = CURSOR_INDICATOR;
      }

      printf("\033[%d;%dH", row, col);
      printf("\033[%dm     \033[0m", bg_color);
      row++;
      printf("\033[%d;%dH", row, col);
      printf("\033[%dm  %s  \033[0m", bg_color, letter);
      row++;
      printf("\033[%d;%dH", row, col);
      printf("\033[%dm  %s  \033[0m", bg_color, cursor_indicator);
    }
  }

  if (FOUND_CANDIDATES != 0) {
    int row = 1;
    int col = (PAD_X * 5 * (GAME_WORD_SIZE + 1)) + PAD_X;
    printf("\033[%d;%dHBulunanlar", row, col);
  }

  for (size_t i = 0; i < FOUND_CANDIDATES; i++) {
    int row = i + 2;
    int col = (PAD_X * 5 * (GAME_WORD_SIZE + 1)) + PAD_X;
    printf("\033[%d;%dH", row, col);

    for (int j = 0; j < GAME_WORD_SIZE; j++) {
      printf("%s", WORDS[CANDIDATE_IDX[i]][j]);
    }
  }
}

void right() {
  if (GAME_CURSOR[1] < GAME_WORD_SIZE - 1) GAME_CURSOR[1]++;
  else {
    if (GAME_CURSOR[0] >= GAME_WORDS_COUNT - 1) return;
    GAME_CURSOR[0]++; GAME_CURSOR[1] = 0;
  }
}

void left() {
  if (GAME_CURSOR[1] > 0) GAME_CURSOR[1]--;
  else {
    if (GAME_CURSOR[0] <= 0) return;
    GAME_CURSOR[0]--; GAME_CURSOR[1] = GAME_WORD_SIZE - 1;
  }
}

void handle_input(input_result_t* input) {
  // printf("%s\n", input->pressed_key);
  if (input->pressed_key == SPACE) {
    cycle_state();
    return;
  }

  if (input->pressed_key == UP) {
    if (GAME_CURSOR[0] > 0) GAME_CURSOR[0]--;
    return;
  }

  if (input->pressed_key == DOWN) {
    if (GAME_CURSOR[0] < GAME_WORDS_COUNT - 1) GAME_CURSOR[0]++;
    return;
  }

  if (input->pressed_key == RIGHT) {
    right();
    return;
  }

  if (input->pressed_key == LEFT) {
    left();
    return;
  }

  if (input->pressed_key == BACKSPACE) {
    left();
    GAME_BOARD[GAME_CURSOR[0]][GAME_CURSOR[1]] = NULL;
    return;
  }

  if (input->pressed_key == DEL) {
    GAME_BOARD[GAME_CURSOR[0]][GAME_CURSOR[1]] = NULL;
    return;
  }

  if (input->pressed_key == ENTER) {
    run_analysis();
    return;
  }

  GAME_BOARD[GAME_CURSOR[0]][GAME_CURSOR[1]] = input->pressed_key;
  right();
}

int main() {
  qsort(INPUT_MAP, sizeof(INPUT_MAP)/sizeof(INPUT_MAP[0]), sizeof(input_member_t), compare_by_size_desc);
  if (!read_words_file()) {
    return 1;
  }

  signal(SIGINT, signal_callback_handler);
  set_buffered_input(0);
  // save term buffer, make cursor invisible, erase entire screen
  printf("\033[?1049h\033[?25l\033[2J");
  draw_board();
  input_result_t* result = malloc(sizeof(input_result_t));
#ifdef DEBUG
  int c;
#endif
  while (1) {
#ifndef DEBUG
    resolve_input(result);
    if (!result->valid) {
      continue;
    }

    if (result->pressed_key == Q) {
      break;
    }

    handle_input(result);
    run_analysis();
    draw_board();
    fflush(stdout);
#else
    c = getchar();

    printf("%d\n", c);
    if (c=='q' || c=='Q') break;
#endif
  }

  set_buffered_input(1);
  // make cursor visible, reset all modes, restore old term buffer
  printf("\033[?25h\033[m\033[?1049l");

  return 0;
}
