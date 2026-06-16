#ifndef TUI_H
#define TUI_H

#include <stdbool.h>

typedef struct {
    int rows;
    int cols;
} TermSize;

TermSize get_term_size(void);
void raw_mode_enable(void);
void raw_mode_disable(void);
int read_key(void);

typedef enum {
    KEY_NONE = 0,
    KEY_UP = -1,
    KEY_DOWN = -2,
    KEY_LEFT = -3,
    KEY_RIGHT = -4,
    KEY_ESC = -5,
    KEY_TAB = -6
} SpecialKey;

#endif
