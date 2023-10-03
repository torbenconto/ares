#ifndef raw
#define raw

#include <termios.h>

extern struct termios orig_termios;

void enableRawMode(void);
void disableRawMode(void);

#endif