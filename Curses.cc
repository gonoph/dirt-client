// this is a seperate interface to all thing cursed
// unfortunately, there is major namespace clash which I can't be bothered fixing

#include <ncurses.h>
#include <term.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "misc.h"
#include "Curses.h"
#include "Color.h"

// Blargh. For non-linux VC output we need to send smacs before and rmacs after the character
char special_chars[max_sc];
char real_special_chars[max_sc];

const char *smacs, *rmacs;

const char* lookup_key(const char *name) {
    const char *x = tigetstr(const_cast<char*>(name));
    if (x == (char*) -1)
        x = NULL;
//    fprintf(stderr, "Lookup %s => %s\n", name, x);
    return x;
}

bool xterm = false; // should set_title work?

void init_curses(bool virtualConsole) {
    int err;
    const char *term = getenv("TERM");
    // note that STDOUT_FILENO is not available for output later on!
    setupterm((char*)term, STDOUT_FILENO, &err);
            
    if (err != 1)
        error ("setupterm: %d. Perhaps your TERM variable is not set correctly?", err);

    // We don't want curses to send any control codes to the terminal, so we
    // send them to the bit bucket
    FILE *fp = fopen ("/dev/null", "r+");
    if (!fp)
        error ("Eeh, can't open /dev/null: %m");
    
    if (!newterm((char*)term, fp, fp))
        error ("error setting up terminal");

    fclose (fp);

    special_chars[bc_vertical] = ACS_VLINE;
    special_chars[bc_horizontal] = ACS_HLINE;
    special_chars[bc_upper_left] = ACS_ULCORNER;
    special_chars[bc_upper_right] = ACS_URCORNER;
    special_chars[bc_lower_left] = ACS_LLCORNER;
    special_chars[bc_lower_right] = ACS_LRCORNER;
    special_chars[sc_filled_box] = ACS_CKBOARD;
    special_chars[sc_half_filled_box] = ACS_DIAMOND;

    // Trickery :( for non-linux terms we need to send smacs/rmacs before
    // so this means the special chars need to be replaced with something special that's later parsed
    // when outputting the border chars
    if (!virtualConsole) {
        memcpy(real_special_chars, special_chars, sizeof(special_chars));
        for (int i = 0; i < max_sc; i++)
            special_chars[i] = SC_BASE + i;
        // Let's choose the character set
        smacs = tigetstr("smacs"); // set ,typically one character
        rmacs = tigetstr("rmacs"); // reset

        // Enable alternate character set
        const char *enacs = tigetstr("enacs");
        if (enacs)
            write(STDOUT_FILENO, enacs, strlen(enacs));
        
    }

    // The change-titlebar capability, is that in
    if (!strcmp(term, "rxvt") || !strcmp(term, "xterm"))
        xterm = true;

}

// not really strictly curses
void set_title(const char *s) {
    if (xterm) {
        char buf[strlen(s) + 64];
        sprintf(buf, "\e]2;%s\007", s);
        write(STDIN_FILENO, buf, strlen(buf));
    }
}

void done_curses() {
}
