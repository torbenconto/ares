#ifndef ares
#define ares

#define ARES_VERSION "0.0.1"

#define SIDE_CHARACTER "~"
#define TAB_STOP 8
#define EXIT_KEY 113 // q

#define CTRL_KEY(k) (k & 0x1f)


struct positions {
    int x;
    int y;
};

void die(const char *e);

#endif