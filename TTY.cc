
#include "dirt.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "Curses.h"
#include "cui.h"
#include "Hook.h"

void TTY::suspend() {
	set_normal();
}

void TTY::restore() {
	set_raw();
}


void TTY::set_raw()
{
	// Initialite the TTY
	long vdisable;
	struct termios	new_term;
	
	if (tcgetattr(STDIN_FILENO, old_term) < 0)
		error ("tty_init:tcgetattr: %m");

	/* Stevens */
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE) < 0))
		error ("fpathconf error or _POSIX_VDISABLE not in effect");		

	new_term = *old_term;
	new_term.c_lflag &= ~(ECHO | ICANON | ISTRIP ); /* echo off, one key at a time */
	new_term.c_iflag &= ~(ICRNL|IGNCR|INLCR|IXON|IXOFF); /* Disable flow control (?) */
	new_term.c_oflag &= ~(OPOST); /* No post processing (?) */
	new_term.c_cc[VMIN] = 0;	/* read at most 1 byte; never block */
	new_term.c_cc[VTIME] = 0;	/* don't wait */

	new_term.c_cc[VSUSP] = vdisable; /* no ^Z */
	new_term.c_cc[VINTR] = vdisable; /* no ^C */
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term) < 0)
		error ("tty_init:tcsetattr: %m");
	
	// Change keypad keys to Application Mode
	write(STDIN_FILENO, "\e=", 2);

}

void TTY::set_normal()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, old_term) < 0)
		error ("tty_reset:tcsetattr: %m");

	// Back to numeric keypad mode
	write(STDIN_FILENO, "\e>", 2);
}


TTY::TTY() : old_term(new struct termios) {
    char *t = ttyname(STDIN_FILENO);
    
    close(STDIN_FILENO);
    if (STDIN_FILENO != open(t, O_RDWR))
        error ("Could not reopen standard input: %m");
    set_raw();
    init_keys();
    last_activity = current_time;
}

TTY::~TTY()
{
	set_normal();	
	delete old_term;
} 

int TTY::init_fdset(fd_set *set, fd_set*) {
	FD_SET(STDIN_FILENO, set);
	return STDIN_FILENO;
}

void TTY::check_fdset(fd_set *set, fd_set*) {
    if (FD_ISSET(STDIN_FILENO,set)) {
        int key = get_key();
        last_activity = current_time;
        embed_interp->set("Key", key);
        hook.run(KEYPRESS, inputLine->get());
        screen->keypress(key); // FIXME KEYPRESS hook instead?
//FIXME        cout << "Activity on keyboard...\n";
    }
}

/* Return a key, or -1 if no key is available */
int TTY::get_raw_key() {
	unsigned char c;
	int count;
	
	while ((count = read(STDIN_FILENO, &c, 1)) < 0)
		if (errno != EINTR && errno != EAGAIN)
			return -1;

	if (count)
		return c;
	else
		return -1;			
}

// Supply the TI key name
#define KEY(name,value) { key_ ## name, #name, value, NULL, NULL },
// Supply the raw key value
#define EKEY(name,value) { key_ ## name, #name, NULL, value, NULL },

// Supply both, either will work
#define KEY2(name,value,value2) { key_ ## name, #name, value, NULL, value2 },

// fucking terminfo/termcap. why isn't the xterm terminfo correct?
static struct
{
	int     code;
	char 	*name;
    char   *cap_string;
    char   *string;
    char   *string2;
}
key_codes[] =
{

EKEY(escape,		"")
KEY2(arrow_up,	"kcuu1", "[A")
KEY2(arrow_down,	"kcud1", "[B")
KEY2(arrow_right,"kcuf1", "[C")
KEY2(arrow_left,	"kkcub1", "[D")
KEY(page_up,	"kpp")
KEY(page_down,	"knp")
KEY2(end,		"kend", "[F")
KEY(home,		"khome")
KEY(insert,		"kich1")
KEY(delete,		"kdch1")
KEY(f1,			"kf1")
KEY(f2,			"kf2")
KEY(f3,			"kf3")
KEY(f4,			"kf4")
KEY(f5,			"kf5")
KEY(f6,			"kf6")
KEY(f7,			"kf7")
KEY(f8,			"kf8")
KEY(f9,			"kf9")
KEY(f10,       "kf10")
KEY(f11,       "kf11")
KEY(f12,       "kf12")
EKEY(pause,		"[P")
EKEY(alt_1,      "1")
EKEY(alt_2,      "2")
EKEY(alt_3,      "3")
EKEY(alt_4,      "4")
EKEY(alt_5,      "5")
EKEY(alt_6,      "6")
EKEY(alt_7,      "7")
EKEY(alt_8,      "8")
EKEY(alt_9,      "9")
EKEY(alt_0,      "0")
EKEY(alt_slash,	"/")
EKEY(alt_a,		"a")
EKEY(alt_b,		"b")
EKEY(alt_c,		"c")
EKEY(alt_d,		"d")
EKEY(alt_e,		"e")
EKEY(alt_f,		"f")
EKEY(alt_g,		"g")
EKEY(alt_h,		"h")
EKEY(alt_i,		"i")
EKEY(alt_j,		"j")
EKEY(alt_k,		"k")
EKEY(alt_l,		"l")
EKEY(alt_m,		"m")
EKEY(alt_n,		"n")
EKEY(alt_o,		"o")
EKEY(alt_p,		"p")
EKEY(alt_q,		"q")
EKEY(alt_r,		"r")
EKEY(alt_s,		"s")
EKEY(alt_t,		"t")
EKEY(alt_u,		"u")
EKEY(alt_v,		"v")
EKEY(alt_w,		"w")
EKEY(alt_x,		"x")
EKEY(alt_y,		"y")
EKEY(alt_z,		"z")
EKEY(kp_enter,	"OM")
EKEY(kp_numlock,	"OP")
EKEY(kp_period,	"On")
EKEY(kp_plus,	"Ol")
EKEY(kp_minus,	"OS")
EKEY(kp_multi,	"OR")
EKEY(kp_divide,	"OQ")
EKEY(kp_0,		"Op")
EKEY(kp_1,		"Oq")
EKEY(kp_2,		"Or")
EKEY(kp_3,		"Os")
EKEY(kp_4,		"Ot")
EKEY(kp_5,		"Ou")
EKEY(kp_6,		"Ov")
EKEY(kp_7,		"Ow")
EKEY(kp_8,		"Ox")
EKEY(kp_9,		"Oy")
KEY(ctrl_a,		"")
KEY(ctrl_b,		"")
KEY(ctrl_c,		"")
KEY(ctrl_d,		"")
KEY(ctrl_e,		"")
KEY(ctrl_f,		"")
KEY(ctrl_g,		"")
KEY(ctrl_h,		"")
KEY(ctrl_i,		"")
KEY(ctrl_j,		"")
KEY(ctrl_k,		"")
KEY(ctrl_l,		"")
KEY(ctrl_n,		"")
KEY(ctrl_o,		"")
KEY(ctrl_p,		"")
KEY(ctrl_q,		"")
KEY(ctrl_r,		"")
KEY(ctrl_s,		"")
KEY(ctrl_t,		"")
KEY(ctrl_u,		"")
KEY(ctrl_v,		"")
KEY(ctrl_w,		"")
KEY(ctrl_x,		"")
KEY(ctrl_y,		"")
KEY(ctrl_z,		"")

	
	{    0, 				NULL, NULL,NULL, NULL		}
};

// lookup certain keys in curses
void TTY::init_keys() {
    for (int i = 0; key_codes[i].name; i++) {
        if (key_codes[i].cap_string != NULL && key_codes[i].cap_string[0]) {
            const char *s = lookup_key(key_codes[i].cap_string);
            if (s && s[0] == '\e')
                key_codes[i].string = strdup(s+1);
            if (key_codes[i].string2 == NULL) {
                s = lookup_key(key_codes[i].cap_string+1); // let's try cuu1 instead of kcuu1
                if (s && s[0] == '\e')
                    key_codes[i].string2 = strdup(s+1);
            }
        }
    }
}

const char *key_name (int key) {
	for (int i=0; key_codes[i].name; i++)
		if (key_codes[i].code == key)
			return key_codes[i].name;
	
	return "unknown-key";
}

int key_lookup (const char *name) {
	for (int i=0; key_codes[i].name; i++)
		if (!strcasecmp(name, key_codes[i].name))
			return key_codes[i].code;

	return -1;			
}			
	


#define MAX_ESCAPE_BUF 64

// Get a key, translating meta stuff
int TTY::get_key() {
	int key = get_raw_key();

	if (key == '\e') { // Probably escape sequence
		char    buf[MAX_ESCAPE_BUF];
		char   *pc = buf;
        int     i;

        // alt-A on certain xterms is 'A' + 0x80
        if (key & 0x80)
            *pc++ = key - 0x80;

		while ((key = get_raw_key()) >= 0 && pc-buf < MAX_ESCAPE_BUF-1)
			*pc++ = key;

		*pc = NUL;

		/* Figure out what control code we are dealing with */
		/* This could be optimized later on. Perhaps use bsearch */
		for (i = 0; key_codes[i].name; i++)
            if ((key_codes[i].string && !strcmp (key_codes[i].string, buf))
                || (key_codes[i].string2 && !strcmp(key_codes[i].string2, buf)))
				return key_codes[i].code;

		return -1; // Unknown combination
	}
	else
		return key;
}

