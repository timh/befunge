// http://catseye.tc/projects/befunge93/doc/website_befunge93.html
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 80
#define HEIGHT 25

typedef enum {
  OP_ADD = '+',          // add top 2
  OP_SUB = '-',          // sub top 2
  OP_MULT = '*',         // mult top 2
  OP_DIV = '/',          // divide top 2
  OP_MOD = '%',          // divide top 2 modulus

  OP_DUP = ':',          // duplicate top stack value
  OP_SWAP = '\\',        // swap top 2
  OP_POP = '$',          // pop top 1

  OP_NOT = '!',          // if top 1 is 0, then 1, else 0.
  OP_GREATERTHAN = '`',  // compare val1,val2, 1 if val1>val2, else 0.
  
  //
  
  OP_DIR_RIGHT = '>',
  OP_DIR_LEFT = '<',
  OP_DIR_UP = '^',
  OP_DIR_DOWN = 'v',
  OP_DIR_RANDOM = '?',

  OP_IF_HORIZ = '_',      // if pop1 true (!= 0), left, else right
  OP_IF_VERT = '|',       // if pop1 true (!= 0), up, else down
  
  OP_STRINGMODE = '"',
  OP_OUT_INT = '.',
  OP_OUT_CHAR = ',',
  OP_IN_INT = '&',
  OP_IN_CHAR = '~',
  
  OP_BRIDGE = '#',        // jump over next command (used for crossings?)
  OP_CODE_GET = 'g',      // get value at code into stack (push 1)
  OP_CODE_PUT = 'p',      // put value from stack at code 

  OP_END = '@',           // end program
  

  OP_BLANK = ' ',
} operation;

typedef enum {
  DIR_RIGHT = 0,
  DIR_DOWN = 1,
  DIR_LEFT = 2,
  DIR_UP = 3
} direction;

typedef struct stack_entry {
  int value;
  struct stack_entry * next;
} stack_entry;

typedef struct runner {
  char * memory;
  int x, y;
  direction dir;
  stack_entry * stack;
  int stringmode;
  int ended;
} runner;

int trim(char * line) {
  int len;
  int trimmed = 0;
  while ((len = strlen(line)) &&
	 (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[len - 1] = '\0';
    ++ trimmed;
  }
  return trimmed;
}

runner * init_runner() {
  runner * result = malloc(sizeof(runner));
  result->memory = (char *) malloc(WIDTH * HEIGHT);
  memset(result->memory, OP_BLANK, WIDTH * HEIGHT);
  result->x = 0;
  result->y = 0;
  result->dir = DIR_RIGHT;
  result->stack = NULL;
  result->stringmode = 0;
  result->ended = 0;
  return result;
}

int read_file_into_memory(runner * run, FILE * in) {
  int result = 0;

  char line[WIDTH + 1];
  int line_no = 1;
  while (fgets(line, sizeof(line), in) != NULL) {
    if ((line_no - 1) > HEIGHT) {
      fprintf(stderr, "line %d: line is beyond maximum height %d\n",
	      line_no, HEIGHT);
      return -1;
    }

    trim(line);

    int line_length = strlen(line);
    if (line_length > WIDTH) {
      fprintf(stderr,
	      "line %d: line has length %d but must be no longer than %d\n",
	      line_no, line_length, WIDTH);
      result = -1;
    }
    else {
      unsigned memory_offset = (line_no - 1) * WIDTH;

      memcpy(run->memory + memory_offset, line, line_length);
    }

    ++ line_no;
  }
  return result;
}

// befunge-93 says popping off an empty stack returns 0, not
// underflow.
int pop_int(runner * run) {
  int result;
  if (run->stack == NULL) {
    result = 0;
  }
  else {
    result = run->stack->value;

    stack_entry * next = run->stack->next;
    free(run->stack);
    run->stack = next;
  }
  return result;
}

void push_int(runner * run, int val) {
  stack_entry * new_entry = (stack_entry *) malloc(sizeof(stack_entry));
  new_entry->value = val;
  new_entry->next = run->stack;
  run->stack = new_entry;
}

int val_at_xy(runner * run, int x, int y) {
  return run->memory[y * WIDTH + x];
}

int val_at_pc(runner * run) {
  return val_at_xy(run, run->x, run->y);
}

int set_val_at_xy(runner * run, int x, int y, int val) {
  return (run->memory[y * WIDTH + x] = val);
}

void move(runner * run) {
  switch (run->dir) {
  case DIR_LEFT:
    run->x -= 1;
    break;

  case DIR_UP:
    run->y -= 1;
    break;

  case DIR_RIGHT:
    run->x += 1;
    break;

  case DIR_DOWN:
    run->y += 1;
    break;

  default:
    fprintf(stderr, "direction set to non-sensical value %d; abort!\n",
	    run->dir);
    exit(1);
  }

  // wrap the x and y after moving.
  if (run->x < 0) {
    run->x += WIDTH;
  }
  run->x %= WIDTH;
  if (run->y < 0) {
    run->y += HEIGHT;
  }
  run->y %= HEIGHT;
}

/* execute the instruction at the current PC (x,y) and move the cursor
 * depending on the direction after executing the current op.
 */
int step(runner * run) {
  int result = 0;
  operation op = run->memory[run->y * WIDTH + run->x];

#if 0
  char stack_str[512] = "[ ";
  stack_entry * e = run->stack;
  while (e != NULL) {
    sprintf(stack_str, "%s%d ", stack_str, e->value);
    e = e->next;
  }
  strcat(stack_str, "]");
  
  printf("(%d, %d): '%c' (%d) :: step :: stack = %s\n",
  	 run->x, run->y,
  	 op, op,
  	 stack_str);
#endif

  if (run->stringmode) {
    if (op == OP_STRINGMODE) {
      run->stringmode = 0;
    }
    else {
      push_int(run, val_at_pc(run));
    }
    move(run);
    return 0;
  }
  
  switch (op) {
  case OP_ADD:
    push_int(run, pop_int(run) + pop_int(run));
    break;

  case OP_SUB: {
    int val1 = pop_int(run);
    int val2 = pop_int(run);
    push_int(run, val2 - val1);
    break;
  }

  case OP_MULT:
    push_int(run, pop_int(run) * pop_int(run));
    break;

  case OP_DIV: {
    int val1 = pop_int(run);
    int val2 = pop_int(run);
    push_int(run, val2 / val1);
    break;
  }

  case OP_MOD: {
    int val1 = pop_int(run);
    int val2 = pop_int(run);
    push_int(run, val2 % val1);
    break;
  }

  case OP_DUP: {
    int in = pop_int(run);
    push_int(run, in);
    push_int(run, in);
    break;
  }

  case OP_SWAP: {
    int val1 = pop_int(run), val2 = pop_int(run);
    push_int(run, val1);
    push_int(run, val2);
    break;
  }

  case OP_POP:
    pop_int(run);
    break;

  case OP_NOT:
    push_int(run, (pop_int(run) == 0) ? 1 : 0);
    break;

  case OP_GREATERTHAN: {
    int val1 = pop_int(run);
    int val2 = pop_int(run);
    push_int(run, val2 > val1 ? 1 : 0);
    break;
  }

  /* directions.. */
  case OP_DIR_RIGHT:
    run->dir = DIR_RIGHT;
    break;

  case OP_DIR_DOWN:
    run->dir = DIR_DOWN;
    break;

  case OP_DIR_LEFT:
    run->dir = DIR_LEFT;
    break;

  case OP_DIR_UP:
    run->dir = DIR_UP;
    break;

  case OP_DIR_RANDOM: {
    int r = rand() % 4;
    run->dir = r;
    break;
  }

  case OP_IF_HORIZ:
    if (pop_int(run) != 0)
      run->dir = DIR_LEFT;
    else
      run->dir = DIR_RIGHT;
    break;

  case OP_IF_VERT:
    if (pop_int(run) != 0)
      run->dir = DIR_UP;
    else
      run->dir = DIR_DOWN;
    break;

  case OP_STRINGMODE:
    run->stringmode = 1;
    break;

  case OP_OUT_INT: {
    char buf[512];
    sprintf(buf, "%d ", pop_int(run));
    write(1, buf, strlen(buf));
    break;
  }

  case OP_OUT_CHAR:
    {
      char buf[1];
      buf[0] = pop_int(run);

      write(1, buf, 1);
    }
    break;

  case OP_IN_INT: {
    int val;
    if (scanf("%d ", &val) < 1) {
      fprintf(stderr, "error at (%d, %d): could not read an int from stdin!\n",
	      run->x, run->y);
      return -1;
    }

    push_int(run, val);
    break;
  }

  case OP_IN_CHAR: {
    char buf[1];
    if (read(0, buf, 1) < 1) {
      fprintf(stderr, "error at (%d, %d): could not read a byte from stdin!\n",
	      run->x, run->y);
      return -1;
    }
    push_int(run, (int) buf[0]);
    break;
  }

  case OP_BRIDGE:
    // extra move; will end up past the thing after the bridge
    move(run);
    break;

  case OP_CODE_GET: {
    int y = pop_int(run);
    int x = pop_int(run);
    int val = val_at_xy(run, x, y);
    push_int(run, val);
    break;
  }

  case OP_CODE_PUT: {
    int y = pop_int(run);
    int x = pop_int(run);
    int val = pop_int(run);
    set_val_at_xy(run, x, y, val);
    break;
  }

  case OP_END:
    run->ended = 1;
    break;

  case OP_BLANK:
    // do nothing.
    break;

  default:
    if (op >= '0' && op <= '9') {
      push_int(run, op - '0');
    }
    else {
      fprintf(stderr,
	      "error at (%d, %d): unknown befunge instruction '%c' (%d)\n",
	      run->x, run->y, op, op);
      return -1;
    }
  }

  // move the PC based on current direction.
  move(run);

  return result;
}

int execute(runner * run) {
  int result = 0;

  while (!run->ended && result == 0) {
    result = step(run);
  }

  return result;
}


int main(int argc, char ** argv) {
  FILE * in;
  if (argc > 1) {
    in = fopen(argv[1], "r");
    if (in == NULL) {
      perror("fopen");
      exit(1);
    }
  }
  else {
    in = stdin;
  }
  
  runner * run = init_runner();
  if (read_file_into_memory(run, in) == -1) {
    exit(1);
  }

  int ret = execute(run);
  printf("\n\n>> ret is %d\n", ret);
}
