#include "editor.h"
#include "ares.h"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

void clearScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

char readKey() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

void processKeypress() {
    char c = readKey();

    switch (c) {
        case CTRL_KEY(EXIT_KEY):
        exit(0);
        break;
    }
}