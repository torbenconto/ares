/*
 * Copyright (c) 2023 Torben Conto
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#ifndef ARES
#define ARES

#define ARES_VERSION "1.0.0"

// Highlight colors, Can be any ansi escape, it's just plugged into an ansi format string
// https://user-images.githubusercontent.com/995050/47952855-ecb12480-df75-11e8-89d4-ac26c50e80b9.png

#define STRING_HIGHLIGHT_COLOR            "38;5;140"
#define NUMBER_HIGHLIGHT_COLOR            "38;5;117"
#define SEARCH_MATCH_HIGHLIGHT_COLOR      "38;5;33"
#define KEYWORD_PRIMARY_HIGHLIGHT_COLOR   "38;5;98"
#define KEYWORD_SECONDARY_HIGHLIGHT_COLOR "38;5;104"
#define COMMENT_HIGHLIGHT_COLOR           "38;5;119"
#define DEFAULT_HIGHLIGHT_COLOR           "32"

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

void die(const char *e);

void setStatusMessage(const char *fmt, ...);
char *ares_prompt(char *prompt, void (*callback)(char *, int));

#endif
