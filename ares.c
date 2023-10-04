#include "includes/buffer.h"
#include "includes/keys.h"
#include "includes/ares.h"
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

struct Config {
  int cx, cy;
  int rx;
  int dirty;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
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

void ares_open(char *filename) {
  free(C.filename);
  C.filename = strdup(filename);
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
    insertRowAt(C.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  C.dirty = 0;
}

char *rowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < C.numrows; j++)
    totlen += C.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < C.numrows; j++) {
    memcpy(p, C.row[j].chars, C.row[j].size);
    p += C.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void save() {
  if (C.filename == NULL) return;
  int len;
  char *buf = rowsToString(&len);
  int fd = open(C.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        C.dirty = 0;
        setStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  setStatusMessage("Can't save! I/O error: %s", strerror(errno));
  free(buf);
}



void scroll() {
    C.rx = 0;
    if (C.cy < C.numrows) {
        C.rx = rowCxToRx(&C.row[C.cy], C.cx);
    }

    if (C.cy < C.rowoff) {
        C.rowoff = C.cy;
    }
    if (C.cy >= C.rowoff + C.screenrows) {
        C.rowoff = C.cy - C.screenrows + 1;
    }
    if (C.rx < C.coloff) {
        C.coloff = C.rx;
    }
    if (C.rx >= C.coloff + C.screencols) {
        C.coloff = C.rx - C.screencols + 1;
    }
}

void drawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    C.filename ? C.filename : "[No Name]", C.numrows,
    C.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
    C.cy + 1, C.numrows);
  if (len > C.screencols) len = C.screencols;
  abAppend(ab, status, len);
  while (len < C.screencols) {
    if (C.screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void drawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(C.statusmsg);
  if (msglen > C.screencols) msglen = C.screencols;
  if (msglen && time(NULL) - C.statusmsg_time < 5)
    abAppend(ab, C.statusmsg, msglen);
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
      int len = C.row[filerow].rsize - C.coloff;
      if (len < 0) len = 0;
      if (len > C.screencols) len = C.screencols;
        abAppend(ab, &C.row[filerow].render[C.coloff], len);
      }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
    
  }
}

void insertRowAt(int at, char *s, size_t len) {
if (at < 0 || at > C.numrows) return;
  C.row = realloc(C.row, sizeof(erow) * (C.numrows + 1));
  memmove(&C.row[at + 1], &C.row[at], sizeof(erow) * (C.numrows - at));  C.row[at].size = len;

  C.row[at].chars = malloc(len + 1);
  memcpy(C.row[at].chars, s, len);
  C.row[at].chars[len] = '\0';

  C.row[at].rsize = 0;
  C.row[at].render = NULL;
  updateRow(&C.row[at]);

  C.numrows++;
  C.dirty++;
}

void updateRow(erow *row) {
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t') tabs++;
  free(row->render);
  row->render = malloc(row->size + tabs*(TAB_STOP - 1) + 1);
  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;
}

void freeRow(erow *row) {
  free(row->render);
  free(row->chars);
}
void delRowAt(int at) {
  if (at < 0 || at >= C.numrows) return;
  freeRow(&C.row[at]);
  memmove(&C.row[at], &C.row[at + 1], sizeof(erow) * (C.numrows - at - 1));
  C.numrows--;
  C.dirty++;
}

void rowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  updateRow(row);
  C.dirty++;
}


void rowInsertCharAt(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  updateRow(row);
  C.dirty++;
}

void rowDelCharAt(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  updateRow(row);
  C.dirty++;
}

void delChar() {
  if (C.cy == C.numrows) return;
  if (C.cx == 0 && C.cy == 0) return;
  erow *row = &C.row[C.cy];
  if (C.cx > 0) {
    rowDelCharAt(row, C.cx - 1);
    C.cx--;
  } else {
    C.cx = C.row[C.cy - 1].size;
    rowAppendString(&C.row[C.cy - 1], row->chars, row->size);
    delRowAt(C.cy);
    C.cy--;
  }
}

void insertChar(int c) {
  if (C.cy == C.numrows) {
    insertRowAt(C.numrows, "", 0);
  }
  rowInsertCharAt(&C.row[C.cy], C.cx, c);
  C.cx++;
}

void insertNewline() {
  if (C.cx == 0) {
    insertRowAt(C.cy, "", 0);
  } else {
    erow *row = &C.row[C.cy];
    insertRowAt(C.cy + 1, &row->chars[C.cx], row->size - C.cx);
    row = &C.row[C.cy];
    row->size = C.cx;
    row->chars[row->size] = '\0';
    updateRow(row);
  }
  C.cy++;
  C.cx = 0;
}

int rowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (TAB_STOP - 1) - (rx % TAB_STOP);
    rx++;
  }
  return rx;
}

void refreshScreen() {
  scroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  drawRows(&ab);
  drawStatusBar(&ab);
  drawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (C.cy - C.rowoff) + 1,
                                            (C.rx - C.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}


void moveCursor(int key) {
  erow *row = (C.cy >= C.numrows) ? NULL : &C.row[C.cy];

  switch (key) {
    case ARROW_LEFT:
      if (C.cx != 0) {
        C.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (row && C.cx < row->size) {
        C.cx++;
      }
      break;
    case ARROW_UP:
      if (C.cy != 0) {
        C.cy--;
      }
      break;
    case ARROW_DOWN:
      if (C.cy < C.numrows) {
        C.cy++;
      }
      break;
  }

  row = (C.cy >= C.numrows) ? NULL : &C.row[C.cy];
  int rowlen = row ? row->size : 0;
  if (C.cx > rowlen) {
    C.cx = rowlen;
  }
}

void setStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(C.statusmsg, sizeof(C.statusmsg), fmt, ap);
  va_end(ap);
  C.statusmsg_time = time(NULL);
}

void processKeypress() {
  int c = readKey();

  switch (c) {
    case '\r':
      insertNewline();
      break;
    case CTRL_KEY(EXIT_KEY):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case HOME_KEY:
      C.cx = 0;
      break;

    case CTRL_KEY('s'):
      save();
      break;

    case END_KEY:
    if (C.cy < C.numrows)
        C.cx = C.row[C.cy].size;
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) moveCursor(ARROW_RIGHT);
      delChar();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
        {
            if (c == PAGE_UP) {
            C.cy = C.rowoff;
            } else if (c == PAGE_DOWN) {
            C.cy = C.rowoff + C.screenrows - 1;
            if (C.cy > C.numrows) C.cy = C.numrows;
            }
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

    case CTRL_KEY('l'):
    case '\x1b':
      break;
    default:
      insertChar(c);
      break;
  }
}

void initEditor() {
  C.cx = 0;
  C.cy = 0;
  C.rx = 0;
  C.numrows = 0;
  C.rowoff = 0;
  C.coloff = 0;
  C.row = NULL;
  C.filename = NULL;
  C.statusmsg[0] = '\0';
  C.statusmsg_time = 0;

  if (getWindowSize(&C.screenrows, &C.screencols) == -1) die("getWindowSize");
  C.screenrows -= 2;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    ares_open(argv[1]);
  }

  setStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");

  while (1) {
    refreshScreen();
    processKeypress();
  }

  return 0;
}
