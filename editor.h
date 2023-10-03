#ifndef editor
#define editor

char readKey();
void processKeypress(void);

int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);

void clearScreen(void);
void resetCursor(void);

void drawRows(int rows);

#endif