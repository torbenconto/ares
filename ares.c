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

void die(const char *e) {
    perror(e);
    exit(1);
}

int main() {  
    enableRawMode();

    for (;;) {
        clearScreen();
        resetCursor();

        processKeypress();
    }
    return 0;
}