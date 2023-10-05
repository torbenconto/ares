#include "ares.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum Keys {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

struct State {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios orig_termios;
};

struct State S;

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &S.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &S.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = S.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
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
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }

    return '\x1b';
  } else {
    return c;
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

int rowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (TAB_STOP - 1) - (rx %  TAB_STOP);
    rx++;
  }
  return rx;
}

int rowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (TAB_STOP - 1) - (cur_rx % TAB_STOP);
    cur_rx++;

    if (cur_rx > rx) return cx;
  }
  return cx;
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

void insertRow(int at, char *s, size_t len) {
  if (at < 0 || at > S.numrows) return;

  S.row = realloc(S.row, sizeof(erow) * (S.numrows + 1));
  memmove(&S.row[at + 1], &S.row[at], sizeof(erow) * (S.numrows - at));

  S.row[at].size = len;
  S.row[at].chars = malloc(len + 1);
  memcpy(S.row[at].chars, s, len);
  S.row[at].chars[len] = '\0';

  S.row[at].rsize = 0;
  S.row[at].render = NULL;
  updateRow(&S.row[at]);

  S.numrows++;
  S.dirty++;
}

void freeRow(erow *row) {
  free(row->render);
  free(row->chars);
}

void delRow(int at) {
  if (at < 0 || at >= S.numrows) return;
  freeRow(&S.row[at]);
  memmove(&S.row[at], &S.row[at + 1], sizeof(erow) * (S.numrows - at - 1));
  S.numrows--;
  S.dirty++;
}

void rowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  updateRow(row);
  S.dirty++;
}

void rowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  updateRow(row);
  S.dirty++;
}

void rowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  updateRow(row);
  S.dirty++;
}

void insertChar(int c) {
  if (S.cy == S.numrows) {
    insertRow(S.numrows, "", 0);
  }
  rowInsertChar(&S.row[S.cy], S.cx, c);
  S.cx++;
}

void insertNewline() {
  if (S.cx == 0) {
    insertRow(S.cy, "", 0);
  } else {
    erow *row = &S.row[S.cy];
    insertRow(S.cy + 1, &row->chars[S.cx], row->size - S.cx);
    row = &S.row[S.cy];
    row->size = S.cx;
    row->chars[row->size] = '\0';
    updateRow(row);
  }
  S.cy++;
  S.cx = 0;
}

void delChar() {
  if (S.cy == S.numrows) return;
  if (S.cx == 0 && S.cy == 0) return;

  erow *row = &S.row[S.cy];
  if (S.cx > 0) {
    rowDelChar(row, S.cx - 1);
    S.cx--;
  } else {
    S.cx = S.row[S.cy - 1].size;
    rowAppendString(&S.row[S.cy - 1], row->chars, row->size);
    delRow(S.cy);
    S.cy--;
  }
}

char *rowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < S.numrows; j++)
    totlen += S.row[j].size + 1;
  *buflen = totlen;

  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < S.numrows; j++) {
    memcpy(p, S.row[j].chars, S.row[j].size);
    p += S.row[j].size;
    *p = '\n';
    p++;
  }

  return buf;
}

void ares_open(char *filename) {
  free(S.filename);
  S.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    insertRow(S.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  S.dirty = 0;
}

void save() {
  if (S.filename == NULL) {
    S.filename = ares_prompt("Save as: %s (ESC to cancel)", NULL);
    if (S.filename == NULL) {
      setStatusMessage("Save aborted");
      return;
    }
  }

  int len;
  char *buf = rowsToString(&len);

  int fd = open(S.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        S.dirty = 0;
        setStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }

  free(buf);
  setStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

void ares_find_cb(char *query, int key) {
  if (key == '\r' || key == '\x1b') {
    return;
  }

  int i;
  for (i = 0; i < S.numrows; i++) {
    erow *row = &S.row[i];
    char *match = strstr(row->render, query);
    if (match) {
      S.cy = i;
      S.cx = rowRxToCx(row, match - row->render);
      S.rowoff = S.numrows;
      break;
    }
  }
}

void ares_find() {
  char *query = ares_prompt("Search: %s (ESC to cancel)", ares_find_cb);
  if (query) {
    free(query);
  }
}

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

void scroll() {
  S.rx = 0;
  if (S.cy < S.numrows) {
    S.rx = rowCxToRx(&S.row[S.cy], S.cx);
  }

  if (S.cy < S.rowoff) {
    S.rowoff = S.cy;
  }
  if (S.cy >= S.rowoff + S.screenrows) {
    S.rowoff = S.cy - S.screenrows + 1;
  }
  if (S.rx < S.coloff) {
    S.coloff = S.rx;
  }
  if (S.rx >= S.coloff + S.screencols) {
    S.coloff = S.rx - S.screencols + 1;
  }
}

void drawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < S.screenrows; y++) {
    int filerow = y + S.rowoff;
    if (filerow >= S.numrows) {
      if (S.numrows == 0 && y == S.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Ares -- version %s", ARES_VERSION);
        if (welcomelen > S.screencols) welcomelen = S.screencols;
        int padding = (S.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = S.row[filerow].rsize - S.coloff;
      if (len < 0) len = 0;
      if (len > S.screencols) len = S.screencols;
      abAppend(ab, &S.row[filerow].render[S.coloff], len);
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void drawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    S.filename ? S.filename : "[No Name]", S.numrows,
    S.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
    S.cy + 1, S.numrows);
  if (len > S.screencols) len = S.screencols;
  abAppend(ab, status, len);
  while (len < S.screencols) {
    if (S.screencols - len == rlen) {
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
  int msglen = strlen(S.statusmsg);
  if (msglen > S.screencols) msglen = S.screencols;
  if (msglen && time(NULL) - S.statusmsg_time < 5)
    abAppend(ab, S.statusmsg, msglen);
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
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (S.cy - S.rowoff) + 1,
                                            (S.rx - S.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void setStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(S.statusmsg, sizeof(S.statusmsg), fmt, ap);
  va_end(ap);
  S.statusmsg_time = time(NULL);
}

char *ares_prompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    setStatusMessage(prompt, buf);
    refreshScreen();

    int c = readKey();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      setStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        setStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
    if (callback) callback(buf, c);
  }
}

void moveCursor(int key) {
  erow *row = (S.cy >= S.numrows) ? NULL : &S.row[S.cy];

  switch (key) {
    case ARROW_LEFT:
      if (S.cx != 0) {
        S.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (row && S.cx < row->size) {
        S.cx++;
      }
      break;
    case ARROW_UP:
      if (S.cy != 0) {
        S.cy--;
      }
      break;
    case ARROW_DOWN:
      if (S.cy < S.numrows) {
        S.cy++;
      }
      break;
  }

  row = (S.cy >= S.numrows) ? NULL : &S.row[S.cy];
  int rowlen = row ? row->size : 0;
  if (S.cx > rowlen) {
    S.cx = rowlen;
  }
}

void processKeypress() {
  static int quit_times = QUIT_TIMES;

  int c = readKey();

  switch (c) {
    case '\r':
      insertNewline();
      break;

    case CTRL_KEY(EXIT_KEY):
      if (S.dirty && quit_times > 0) {
        // TODO: use EXIT_KEY in prompt
        setStatusMessage("File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case CTRL_KEY('s'):
      save();
      break;

    case HOME_KEY:
      S.cx = 0;
      break;

    case END_KEY:
      if (S.cy < S.numrows)
        S.cx = S.row[S.cy].size;
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) moveCursor(ARROW_RIGHT);
      delChar();
      break;

    case CTRL_KEY('f'):
      ares_find();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          S.cy = S.rowoff;
        } else if (c == PAGE_DOWN) {
          S.cy = S.rowoff + S.screenrows - 1;
          if (S.cy > S.numrows) S.cy = S.numrows;
        }

        int times = S.screenrows;
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

  quit_times = QUIT_TIMES;
}

void initEditor() {
  S.cx = 0;
  S.cy = 0;
  S.rx = 0;
  S.rowoff = 0;
  S.coloff = 0;
  S.numrows = 0;
  S.row = NULL;
  S.dirty = 0;
  S.filename = NULL;
  S.statusmsg[0] = '\0';
  S.statusmsg_time = 0;

  if (getWindowSize(&S.screenrows, &S.screencols) == -1) die("getWindowSize");
  S.screenrows -= 2;
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