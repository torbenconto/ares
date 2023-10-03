#ifndef editor
#define editor

#include "ares.h"

int readKey();
void processKeypress(struct positions p, int rows);

int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);

void clearScreen(void);
void resetCursor(void);

void drawRows(int rows, int cols);

void moveCursor(int key, struct positions p);

#endif