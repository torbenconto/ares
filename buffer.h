#ifndef buffer
#define buffer

#define APBUF_INIT {NULL, 0}

struct apbuffer {
    char *b;
    int len;
};

#endif