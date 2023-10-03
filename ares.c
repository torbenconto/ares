#include "includes/ares.h"
#include "includes/keys.h"
#include "includes/buffer.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

typedef struct erow {
  int size;
  char *chars;
} erow;

struct Config {
  int cx, cy;
  int rowoff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  struct termios orig_termios;
};

struct Config C;

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &C.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &C.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = C.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void open(char *filename) {
 FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  linelen = getline(&line, &linecap, fp);
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    appendRow(line, linelen);
  }
  free(line);
  fclose(fp);
}

void scroll() {
    if (C.cy < C.rowoff) {
        C.rowoff = C.cy;
    }
    if (C.cy >= C.rowoff + C.screenrows) {
        C.rowoff = C.cy - C.screenrows + 1;
    }
}


void drawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < C.screenrows; y++) {
    int filerow = y + C.rowoff;
    if (filerow >= C.numrows) {
      if (C.numrows == 0 && y == C.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Ares -- version %s", ARES_VERSION);
        if (welcomelen > C.screencols) welcomelen = C.screencols;
        int padding = (C.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, SIDE_CHARACTER, 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, SIDE_CHARACTER, 1);
      }
    } else {
      int len = C.row[filerow].size;
      if (len > C.screencols) len = C.screencols;
      abAppend(ab, C.row[filerow].chars, len);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < C.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void appendRow(char *s, size_t len) {
  C.row = realloc(C.row, sizeof(erow) * (C.numrows + 1));
  int at = C.numrows;
  C.row[at].size = len;
  C.row[at].chars = malloc(len + 1);
  memcpy(C.row[at].chars, s, len);
  C.row[at].chars[len] = '\0';
  C.numrows++;
}

void refreshScreen() {
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  drawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", C.cy + 1, C.cx + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}


void moveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
      if (C.cx != 0) {
        C.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (C.cx != C.screencols - 1) {
        C.cx++;
      }
      break;
    case ARROW_UP:
      if (C.cy != 0) {
        C.cy--;
      }
      break;
    case ARROW_DOWN:
      if (C.cy != C.screenrows - 1) {
        C.cy++;
      }
      break;
  }
}

void processKeypress() {
  int c = readKey();

  switch (c) {
    case CTRL_KEY(EXIT_KEY):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case HOME_KEY:
      C.cx = 0;
      break;

    case END_KEY:
      C.cx = C.screencols - 1;
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        int times = C.screenrows;
        while (times--)
            moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      moveCursor(c);
      break;
  }
}

void initEditor() {
  C.cx = 0;
  C.cy = 0;
  C.numrows = 0;
  C.rowoff = 0;
  C.row = NULL;

  if (getWindowSize(&C.screenrows, &C.screencols) == -1) die("getWindowSize");
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    open(argv[1]);
  }

  while (1) {
    refreshScreen();
    processKeypress();
  }

  return 0;
}