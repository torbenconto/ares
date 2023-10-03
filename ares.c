/*
    Author: Torben Conto
    Publishing Date: 10/3/2023
    License: MIT

    following and adding on to https://viewsourcecode.org/snaptoken/kilo in order to learn C
*/

#include "raw.h"
#include "ares.h"
#include "editor.h"
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

struct positions p;

void die(const char *e) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    
    perror(e);
    exit(1);
}

int main() {  
    p.x = 0;
    p.y = 0;

    enableRawMode();

    int x, y;
    if (getWindowSize(&x, &y) == -1) die("getWindowSize");


    for (;;) {
        write(STDOUT_FILENO, "\x1b[?25l", 6);

        resetCursor();

        drawRows(x, y);
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", p.y + 1, p.x + 1);
        write(STDOUT_FILENO, buf, strlen(buf));

        resetCursor();

        write(STDOUT_FILENO, "\x1b[?25h", 6);

        processKeypress(p, x);
    }
    return 0;
}