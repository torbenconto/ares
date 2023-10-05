#ifndef ares
#define ares

#define ARES_VERSION "0.0.1"

// TODO: 256 ANSI color code support
#define STRING_HIGHLIGHT_COLOR 35
#define NUMBER_HIGHLIGHT_COLOR 36
#define SEARCH_MATCH_HIGHLIGHT_COLOR 34
#define COMMENT_HIGHLIGHT_COLOR 32
#define DEFAULT_HIGHLIGHT_COLOR 31

#define SIDE_CHARACTER "~"
#define TAB_STOP 8
#define QUIT_TIMES 1
#define EXIT_KEY 113 // q
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)
#define CTRL_KEY(k) (k & 0x1f)

struct positions {
    int x;
    int y;
};

void die(const char *e);
void setStatusMessage(const char *fmt, ...);
void refreshScreen();
char *ares_prompt(char *prompt, void (*callback)(char *, int));
#endif