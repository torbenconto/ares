#ifndef ARES
#define ARES

#define ARES_VERSION "0.0.1"

// Highlight colors
#define STRING_HIGHLIGHT_COLOR          35
#define NUMBER_HIGHLIGHT_COLOR          36
#define SEARCH_MATCH_HIGHLIGHT_COLOR    34
#define KEYWORD_PRIMARY_HIGHLIGHT_COLOR 33
#define KEYWORD_SECONDARY_HIGHLIGHT_COLOR 32
#define COMMENT_HIGHLIGHT_COLOR         32
#define DEFAULT_HIGHLIGHT_COLOR         31

// Configuration settings
#define SIDE_CHARACTER      "~"
#define TAB_STOP            8
#define TAB_WIDTH           2
#define QUIT_TIMES          1
#define EXIT_KEY            113  // q

// Highlight flags
#define HL_HIGHLIGHT_NUMBERS    (1 << 0)
#define HL_HIGHLIGHT_STRINGS    (1 << 1)

// Function to clear the CTRL key
#define CTRL_KEY(k) (k & 0x1f)

// Data structure for positions
struct positions {
    int x;
    int y;
};

// Function prototypes
void die(const char *e);
void setStatusMessage(const char *fmt, ...);
void refreshScreen();
char *ares_prompt(char *prompt, void (*callback)(char *, int));

#endif
