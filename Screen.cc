// The Screen class is a derivative of Window that writes the resulting
// data onto the physical screen

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/socket.h> // Some libc/linux versions seem to have iovec there :(
#include "dirt.h"
#include "cui.h"
#include "Buffer.h"
#include "Curses.h"
#include "Session.h"
#include "Hook.h"

// This isn't too pretty
int get_screen_size_x()
{
	// Screen dimensions
	struct winsize size;
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0)
		error ("get_screen_size:ioctl:%m");

	return size.ws_col;		
}

int get_screen_size_y()
{
	// Screen dimensions
	struct winsize size;
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0)
		error ("get_screen_size:ioctl:%m");

	return size.ws_row;		
}
		
		

// Initialization. Figure out what dimensions we have here 
Screen::Screen() : Window(NULL, get_screen_size_x(), get_screen_size_y()), fd(0) {
    // Open our fd
    char *tty = ttyname(STDIN_FILENO);
    int ttyno;
    char buf[256];
    usingVirtual = true;

    // pre 2.4
    if (1 == (sscanf(tty, "/dev/tty%d", &ttyno)))
        sprintf (buf, "/dev/vcsa%d", ttyno);
    // 2.4 with devs (noted by moon@deathmoon.com
    else if (1 == (sscanf(tty, "/dev/vc/%d", &ttyno)))
        sprintf (buf, "/dev/vcc/a%d", ttyno);
    else {
        usingVirtual = false;
        scr_x = scr_y = scr_w = scr_h = 0;
        last_screen = new attrib[width * height];
        memcpy(last_screen, canvas, width * height * sizeof(attrib));
        write(STDOUT_FILENO, CLEAR_SCREEN, strlen(CLEAR_SCREEN));
        out = new Buffer(32000);
    }

    if (usingVirtual) {
      if ((fd = open (buf, O_WRONLY)) < 0) {
	fprintf (stderr, "\nFailed to open %s: %m. \nPerhaps your permissions are wrong?\n\n", buf);
	exit (EXIT_FAILURE);
      }
    }

    init_curses(usingVirtual);
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, "Screen::keypress_ctrl_l",
        vector<string>(1, "Dirt keys"), "", "", NULL, key_ctrl_l, "", &Screen::keypress_ctrl_l, (void*)this));
}

// Refresh the screen
bool Screen::refreshVirtual() {
    unsigned char cursor_pos[2];
    
    struct iovec write_struct[2] =
    {
        {(void*) &cursor_pos,	2},
        {(void*) canvas,		width*height*sizeof(*canvas)}
    };
    
    if (dirty)
        clear();
    
    if (Window::refresh())
    {
        
        lseek(fd,2,SEEK_SET);	// Go to beginning of the vcsa, but not the first
        // 2 bytes which are selection pos or somethign
        
        // Write cursor position, then canvas data to the device
        
        cursor_pos[0] = cursor_x;
        cursor_pos[1] = cursor_y;
        
        while (writev (fd, write_struct, 2) < 0)
            if (errno != EINTR && errno != EAGAIN)
                error ("Screen::refresh():writev");
        
        return true;
    }
    else
        return false;
}

bool Screen::refresh() {
    if (usingVirtual)
        return refreshVirtual();
    else
        return refreshTTY();
}

// Why this?
// ANSI colors and the colors used by /dev/vcsa do not fit together
// Internally we use the /dev/vcsa colors so we must translate here
static int reverse_color_conv_table[8] =  {
    0,4,2,6,1,5,3,7
};

// 8-bit CSI - use this if your terminal supports it!
// #define CSI "\x9B"
// 7-bit CSI
#define CSI "\e["

// TODO: split it up so we can change fg and bg seperately
const char* Screen::getColorCode (int color, bool setBackground) const {
    int fg_color = 30 + reverse_color_conv_table[color & 0x07];
    int bold = color & fg_bold;
    int bg_color = 40 + reverse_color_conv_table[color >> 4];
    static char buf[64];
    
    // Xterm doesn't like fg_white|bg_black ?!
    if (fg_color == 37 &&  bg_color == 40 && !bold)
        sprintf(buf, CSI "0m");
    else {
        char bg[8];
        if (setBackground)
            sprintf(bg, "%d;", bg_color);
        else
            bg[0] = NUL;
        
        if (bold)
            sprintf(buf, CSI "1;%s%dm", bg, fg_color);
        else
            sprintf(buf, CSI "0;%s%dm", bg, fg_color);
    }

    return buf;
}

void Screen::setTTYColor(int color, bool setBackground) {
    const char *code = getColorCode(color, setBackground);
    globalStats.ctrl_chars += out->strcat(code);
}

// Print a single character. If it's a graphical character, and ACS is not yet enabled it, enable it
void Screen::printCharacter(int c, bool& acs_enabled) {
    if (c >= SC_BASE && c < SC_END && smacs) {
        if (!acs_enabled)
            out->strcat(smacs);
        out->chrcat(real_special_chars[c-SC_BASE]);
        acs_enabled = true;
    } else {
        if (acs_enabled && rmacs) {
            out->strcat(rmacs);
            acs_enabled = false;
        }
        out->chrcat(c);
    }
}


// This is Y,X, both starting at 1
#define VT_GOTO CSI "%d;%dH"
#define VT_CLEAR_EOL CSI "K"

// goes to 1,1
#define VT_HOME CSI "H" 
#define MAX_SCROLL_TRY 1

// some more optimization could be useful here!
// Checksum rows? (could error)
// do a memcp before running through it?
// probably not worth it, it's the output that's most important
bool Screen::refreshTTY() {
    int x,y;
    bool acs_enabled = false;
    
    if(!Window::refresh()) return false;
        
    // let's go with a very raw refresh for now
    out->clear();

    // how many lines are different? FIXME scan for \n in input instead?  This is inefficent.
    int diff = 0;
    for (y = 0; y < height; y++)
        if (memcmp(last_screen+y*width, canvas+y*width, width * sizeof(attrib)) != 0)
            diff++;

    // Figure out whether we can scroll the scrolling region
    // Note that it isn't always a win.. when e.g. the alt-o window is open for example, how to evaluate?
    if (scr_h && diff > width/2) {
        int scrreg_bottom = scr_y + scr_h - 1; // the last line of the scrolling region
        int scrreg_test_start = scr_y + 5; // Where do we start looking to determine scroll happened?
        int scrreg_test_end =  scrreg_test_start + MAX_SCROLL_TRY;
        
        // find out where the saved line at the bottom of the scrreg has went to
        for (y = scrreg_test_start; y <= scrreg_test_end ; y++) {
            if (memcmp(last_screen + scrreg_test_end*width, canvas + y*width, width * sizeof(attrib)) == 0) {
                int lines = scrreg_test_end - y;
                if (lines > 0) {

                    // adjust to what we think the screen looks like
                    memmove(last_screen + scr_y*width, last_screen+ ((scr_y+lines)*width), 
                        (scr_h - lines) * width * sizeof(attrib));
                    
                    // mark the black that should now appear at the bottom there
                    for (y = scrreg_bottom - lines; y <= scrreg_bottom; y++)
                        memcpy(last_screen + y*width, clearLine, width * sizeof(attrib));
                    
                    setTTYColor(bg_black|fg_white, true); // we want to scroll up black space
                    globalStats.ctrl_chars += out->printf(CSI "%d;%dr", scr_y+1, scr_y+scr_h);
                    globalStats.ctrl_chars += out->printf(VT_GOTO, scr_y+scr_h, 1);
                    while (lines-- > 0)
                        out->strcat("\n");
                    globalStats.ctrl_chars += out->printf(CSI "%d;%dr", 1, height);
                }
                break;
            }
        }
    }
    
    out->printf(VT_HOME); // top left corner

    int saved_color = -1;
    int last_y = 0, last_x = 0;
    for (y = 0; y < height ; y++) {
        for (x = 0; x < width; x++) {

            // don't write in that very last cell FIXME why?
            if (y == height-1 && x == width-1) continue;
            
            int offset = x + y * width;
            if (canvas[offset] != last_screen[offset]) {
                int color = canvas[offset] >> 8;
                int c = canvas[offset] & 0xFF;
                if (color != saved_color) {

                    // XTerm seems to have problems with this?!
#if 0
                    setTTYColor(color, color >> 4 != saved_color >> 4);
#endif
                    setTTYColor(color, true);
                    saved_color = color;
                }

                // Are we there yet?
                if (x != last_x || y != last_y) {

                    // It is  more efficient to just print the actual skipped character in some
                    // cases rather than goto (especially when it's e.g. a space in our current color)
                    if (last_y == y && last_x == x-1 && (canvas[offset-1] >> 8) == saved_color)
                        printCharacter(canvas[offset-1] & 0xFF, acs_enabled);
                    else {
                        // just on the line above? then we can just send a newline to go down
// FIXME was 1, but this causes screen corruption
#if 0 
                        if (x == 0 && last_y == y-1)
                            out->printf("\r\n");
                        else
#endif
                        globalStats.ctrl_chars += out->printf(VT_GOTO, y+1, x+1);
                    }
                }
                
                last_y = y;
                last_x = x+1;
                if (last_x == width) {
                    last_x = 0;
                    last_y++;
                }
                
                if (c >= 32)
                    printCharacter(c, acs_enabled);
                else
                    out->chrcat(' ');
            }
        }
    }
    globalStats.ctrl_chars += out->printf(VT_GOTO, cursor_y+1, cursor_x+1); // top left corner

    if (rmacs && acs_enabled) // Always reset JIC
        globalStats.ctrl_chars += out->strcat(rmacs);
    
    
    int n = write(STDIN_FILENO, ~*out, out->count());
    if (n != out->count())
        abort();
    
    memcpy(last_screen, canvas, width*height*sizeof(attrib));
    
    globalStats.tty_chars += out->count();
    
    return true;
}

Screen::~Screen() {
    if (usingVirtual) close (fd);
}

void Screen::set_cursor(int _x, int _y) {
	cursor_x = _x;
	cursor_y = _y;
}

void Screen::setScrollingRegion(int x, int y, int w, int h) {
    scr_x = x; scr_y = y; scr_w = w; scr_h = h;
}

void Screen::flash() {
    if (config->getOption(opt_beep))
        write (STDIN_FILENO, "\a",1);
}

bool Screen::keypress(int key) {
    // Check for alt-X
    if (key >= key_alt_1 && key < key_alt_9)  {
        Window *w = Numbered::find(key-key_alt_1+1);
        if (w) {
            w->show(true);
            w->popUp();
            return true;
        }
    }
    
    if (!this->Window::keypress(key)) // this appears to get hosed in this call.
        if (!currentSession || !currentSession->expand_macros(key))
            status->setf ("Keycode %d (%s) was not handled by any object", key, key_name(key));

    return true;
}

// redraw
bool Screen::keypress_ctrl_l(string&, void* mt) {
    Screen* mythis = (Screen*)mt;
    if (!mythis->usingVirtual) memset(mythis->last_screen, 0, mythis->width*mythis->height*sizeof(attrib));
    mythis->dirty = true;
    return true;
}
