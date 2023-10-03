#ifndef config
#define config

#include "io.h"
#include <termios.h>

struct Config {
  int cx, cy;
  int screenrows;
  int screencols;
  int numrows;
  erow row;
  struct termios orig_termios;
};


#endif