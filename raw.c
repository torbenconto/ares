#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include "raw.h"

struct termios orig_termios;

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios rawmode = orig_termios;

  // Disable software control flow inputs
  rawmode.c_iflag &= ~(IXON);

  // Disable echo, canonical mode, and SIG keys like CTRL + C
  rawmode.c_lflag &= ~(ECHO | ICANON | ISIG);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawmode);
}