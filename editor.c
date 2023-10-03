#include "editor.h"
#include "ares.h"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
};

void clearScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

void resetCursor() {
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void moveCursor(int key, struct positions p) {
  switch (key) {
    case ARROW_LEFT:
      p.x--;
      break;
    case ARROW_RIGHT:
      p.x++;
      break;
    case ARROW_UP:
      p.y--;
      break;
    case ARROW_DOWN:
      p.y++;
      break;
  }
}

void drawRows(int rows, int cols) {
    for (int y = 0; y < rows; y++) {
        if (y == rows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                "Ares text editor -- version %s", ARES_VERSION);
        if (welcomelen > cols) welcomelen = cols;
              int padding = (cols - welcomelen) / 2;
            if (padding) {
                write(STDOUT_FILENO, "~", 1);
                padding--;
            }
            while (padding--) write(STDOUT_FILENO, " ", 1);
            write(STDOUT_FILENO, welcome, welcomelen);
        } else {
            write(STDOUT_FILENO, "~", 1);
        }

        write(STDOUT_FILENO, "\x1b[K", 3);
        if (y < rows - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

int readKey() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

void processKeypress(struct positions p) {
    int c = readKey();

    switch (c) {
        case CTRL_KEY(EXIT_KEY):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            moveCursor(c, p);
            break;
    }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}