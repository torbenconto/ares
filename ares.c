#include "raw.h"
#include "ares.h"
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

int main() {  
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != EXIT_KEY) {
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
    }
    return 0;
}