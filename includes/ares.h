#ifndef ares
#define ares

#define ARES_VERSION "0.0.1"

#define CTRL_KEY(k) (k & 0x1f)
#define EXIT_KEY 113 // q

struct positions {
    int x;
    int y;
};

void die(const char *e);

#endif