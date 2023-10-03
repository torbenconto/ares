#ifndef raw
#define raw

struct termios orig_termios;

void enableRawMode();
void disableRawMode();

#endif