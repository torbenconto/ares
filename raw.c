#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include "raw.h"
#include "ares.h"

struct termios orig_termios;

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios rawmode = orig_termios;

  // Disable software control flow inputs and fix CTRL + M, trim last byte of chars
  rawmode.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

  // Change character size to 8 bytes
  rawmode.c_cflag |= (CS8);

  // Disable output processing
  rawmode.c_oflag &= ~(OPOST);

  // Disable echo, canonical mode, CTRL + V, and SIG keys like CTRL + C
  rawmode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawmode) == -1) die("tcsetattr");
}