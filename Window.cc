#include "dirt.h"
#include "Window.h"
#include "ProxyWindow.h"
#include "Numbered.h"
#include "MessageBox.h"
#include "Curses.h"
#include "Screen.h"

#include <stdarg.h>
#include <deque>
#include <list>
#include <iostream>

using namespace std;

Window* Numbered::list[10];

Window::Window(Window *_parent, int _width, int _height,  Style _style, int _x, int _y)
    : parent(_parent), width(_width), height(_height),
    visible(true), color(fg_white|bg_black),focused(NULL), parent_x(_x), 
    parent_y(_y), cursor_x(0), cursor_y(0), dirty(true),  style(_style)
{
//    cout << "Window::Window(" << _parent << "," << _width << "," << _height << "," << _style << "," << _x << "," << _y << ")" << endl;
    if (width == wh_half)
        width = parent->width/2;
    else if (width == wh_full)
        width = parent->width;
    
    if (height == wh_half)
        height = parent->height/2;
    else if (height == wh_full)
        height = parent->height;

    if (parent_x == xy_center)
        parent_x = parent->width / 2 - width / 2;
    else if (parent_x < 0)
        parent_x = parent->width + parent_x;
            
    if (parent_y == xy_center)
        parent_y = parent->height / 2 - height / 2;
    else if (parent_y < 0)
        parent_y = parent->height + parent_y;

    // Bordered?
    if (style & Bordered) {
        Window *w = new Border(parent, width, height, parent_x, parent_y);
        width -= 2;
        height -= 2;
        parent_x = parent_y = 1 ;
        parent = w;
    }

    assert(width >= 0);
    assert(height >= 0);

    clearLine = new attrib[width];
    for (attrib *a = clearLine; a < clearLine+width; a++)
        *a =  ((fg_white|bg_black) << 8) + ' ';
    
    canvas = new attrib[width*height];
    clear();

    if (parent) parent->insert(this);
    if(name.empty()) name = "Window-unknown";
}

bool Window::is_focused() {
//        if (parent && parent->focused && parent->focused == this)// maybe this instead?
    if (!parent || !parent->focused || parent->focused == this)  // FIXME this logic seems wrong...
        return true;
    else
        return false;
}

void Window::insert (Window *window) {
    if (find(window))
        error("Window::insert, window not found\n");
        
    check();
    for(list<Window*>::iterator it = children.begin();it != children.end();it++) {
        if(*it == window) {
            report_err("Window::insert attempting to insert a window twice (1)!\n");
            return;
        }
    }

    assert(window != NULL);
    children.push_back(window);
    assert(window->parent == this);
    check();
}

void Window::check() {
    int count=0;

    for(list<Window*>::iterator it = children.begin();it != children.end();it++) {
        if(*it == NULL) error("Window::check found a NULL child!\n");
        if (++count > 100)  {
            error("Window::check too many children\n");
        }
    }
}

void Window::show(bool vis) {
    visible = vis;
    dirty = true;
    if (parent)
        parent->visibilityNotify(this, vis);
}

void Window::remove (Window *window)
{
    if (!find(window)) {
        report_err("Window::remove unable to remove window because it is not a child!");
        return;
    }
    
    for(list<Window*>::iterator it = children.begin();it != children.end();it++) {
        if(*it == window) { it = children.erase(it); break; }
    }
    dirty = true;
    if (focused) focused->dirty = true;
    if (window == focused) focused = NULL;
    
    check();
}

Window::~Window()
{
    check();
    // Remove me
    if (parent) { parent->remove(this); }
    list<Window*>::iterator it;
    while(!children.empty()) { // After delete, iterators are invalid (due to child calling parent->remove)
        it = children.begin();
        assert(*it != parent); // This fails sometimes too!
        assert(*it != this);
        assert(this != parent);
        assert(*it != NULL);
        if(*it) { // This should never be null, but check anyway.
            assert(this == (*it)->parent); // This fails sometimes.  FIXME WHY?
            (*it)->die(); // calls delete
        } else {
            error("NULL child in list!\n");
        }
    }
            
    delete[] canvas;
}

Window* Window::findByName(const char *name)
{
    for(list<Window*>::iterator it = children.begin();it != children.end();it++)
        if (!strcmp(name, (*it)->getName()))
            return *it;
    
    return NULL;
}

bool Window::scroll()
{
    return false;
}

#define MAX_PRINTF_BUF 16384

void Window::print(const char *s)
{
    const unsigned char *in;
    attrib *out;
	
    dirty = true;

    // Position out pointer at the beginning of canvas to write to	
    out = canvas + cursor_y * width + cursor_x;
    
    for (in = (unsigned char *)s; *in; in++)
    {
        if (*in == SET_COLOR) {
            switch (in[1]) {
            case 0: // ??? black on black?
                break;
                
            case COL_SAVE:
                saved_colors.push_front(color);
               // saved_color = color; 
                in++;
                break;

            case COL_RESTORE:
                if(!saved_colors.empty()) { 
                    color = saved_colors.front();
                    saved_colors.pop_front();
                }
               // color = saved_color; 
                in++;
                break;

            default:
                color = *++in;
            }
        } else if (*in == SOFT_CR) { // change to a new line if not there already
            if (cursor_x) {
                cursor_x = 0; 
                cursor_y++;
                out = canvas + cursor_y * width + cursor_x;
            }
        } else if (*in == '\t') { // tabstop
            // Hop to the next tabstop. If we go over the limit, set cursor
            // to be = width, then wordwrap will kick in below
            cursor_x = min(width, ((cursor_x / config->getOption(opt_tabsize)) + 1) 
                * config->getOption(opt_tabsize));
        } else if (*in == '\n') { // Change line
            cursor_x = 0; 
            cursor_y++;
            out = canvas + cursor_y * width + cursor_x;
            // FIXME do scrolling here?
            while (cursor_y > height)
                if (!scroll())
                    return;
        } else {
            // Need to scroll? If we have to, and fail, don't print more
            // If scroll succeeds, it is assumed to have adjusted cursor_x/y
            // Note that we do not scroll until we actually WRITE something
            // at the bottom line!
            while (cursor_y >= height)
                if (!scroll())
                    return;
                
            // Wordwrap required?
            if (cursor_x > width-1)
            {
                cursor_y++;
                cursor_x = 0;
            }
            // Recalculate where our output is now that there has been a scroll	
            out = canvas + cursor_y * width + cursor_x;
            
            *out++ = (color << 8) + *in;
            cursor_x++;
        }
    }
}

// Formatted print
void Window::printf(const char *fmt, ...)
{
    char buf[MAX_PRINTF_BUF];
    va_list va;
    
    va_start (va,fmt);
    vsnprintf (buf, MAX_PRINTF_BUF, fmt, va);
    va_end (va);
    
    print(buf);
}

// Formatted, centered print
void Window::cprintf(const char *fmt, ...)
{
    char buf[MAX_PRINTF_BUF];
    va_list va;
	
    va_start (va,fmt);
    vsnprintf (buf, MAX_PRINTF_BUF, fmt, va);
    va_end (va);

    gotoxy(0, cursor_y);
    if ((int)strlen(buf) > width)
        buf[width] = NUL;

    printf("%*s%s", (width-strlen(buf))/2, "", buf);
}

// Copy data from attrib to x,y. Data is w * h
void Window::copy (attrib *source, int w, int h, int _x, int _y)
{
	// Don't bother if it is clearly outside of our rectangle
    if (_y >= width || _x >= width || (_x+w <= 0) || (_y+h) <= 0) // far out
		return;
		
    // Direct copy?
    if (width == w && _x == 0) {
        // adjust if _y < 0
        if (_y < 0) {
            source += (-1 * w*_y);
            h += _y;
            _y = 0;
        }
        int size = min(w*h, (height-_y) * width);
		
    // This will not copy more than there is space for in this window
        if (size > 0)
            memcpy (canvas + _y*width, source, size * sizeof(attrib));
    } else {
        // For each row up to max of our height or source height
        for (int y2 = max(0,_y); y2 < height && y2 < (_y+h) ; y2++)
            memcpy (
                canvas + (y2 * width) + _x,				// Copy to (_x, y2)
                source + (y2 - _y) * w,					// Copy from (0, y2 - _y)
                sizeof(attrib) * min(w, width - _x));	// Copy w units, or whatever there's space for
    }
}

// This simple window cannot redraw itself
// If the canvas somehow is lost, printed stuff is lost
void Window::redraw()
{
    dirty = false;
}

bool Window::refresh()
{
    bool refreshed = false;

    // Don't do anything if we are hidden	
    if (!visible) {
        if (dirty) {
            dirty = false;
            return true;
        } else return false;
    }
	
    // Do we need to redraw?
    if (dirty) {
        redraw();
        refreshed = true;
    }
	
    // Refresh children
    // Note that the first in list will be the one behind
    for(list<Window*>::iterator it = children.begin();it != children.end();it++)
        refreshed = (*it)->refresh() || refreshed;

    draw_on_parent();		 // Copy our canvas to parent

    return refreshed;		
}

// Fill this window with 'color'
void Window::clear()
{
    attrib *p;
    attrib *end = canvas + width * height;
    attrib blank = (color << 8) + ' ';
    
    for (p = canvas; p < end; p++)
        *p = blank;
    
    dirty = true;
}

// Take care of this keypress
bool Window::keypress(int key)
{
    // By default, do not do anything with the key, but just ask the children
    // If they can do anything with it (starting with most recently added child)
    for(list<Window*>::reverse_iterator rit = children.rbegin();rit != children.rend();rit++) {
        if ((*rit)->keypress(key)) return true;
    }
    
    return false;
}

// Set hardware cursor coordinates
// Asks the parent, translating the coordinates accordingly
void Window::set_cursor(int _x, int _y)
{
    if (parent)
        parent->set_cursor(_x+parent_x,_y+parent_y);
}


void Window::idle()
{
    for(list<Window*>::iterator it = children.begin();it != children.end();it++) {
        (*it)->idle();
    }
}

void Window::resize (int w, int h) {
    width = w; height = h;
    delete[] canvas;
    canvas = new attrib[width*height];
    clear();
    if (parent) {
        parent->force_update();
        parent->resizeNotify(this, w,h);
    }
    dirty = true;
}

void Window::move (int x, int y) {
    parent_x = x; parent_y = y;
    force_update();
    parent->force_update();
}
        


#define BRD_TOP 		(1 << 0)
#define BRD_BOTTOM 		(1 << 1)
#define BRD_LEFT 		(1 << 2)
#define BRD_RIGHT 		(1 << 3)


// Draw a box from (x1,y2) to (x2,y2) using border style _borders
void    Window::box (int x1, int y1, int x2, int y2, int _borders)
{
    int     box_width = x2 - x1;

    char   *s = new char[box_width + 1];

    // Top/bottom   
    memset (s, special_chars[bc_horizontal], box_width);
    s[box_width] = NUL;

    if (_borders & BRD_TOP) {
        gotoxy (x1 + 1, y1);
        print (s);
    }

    if (_borders & BRD_BOTTOM) {
            gotoxy (x1, y2);
            print (s);
    }

    // Left/Right
    for (int _y = y1 + 1; _y < y2; _y++) {
        if (_borders & BRD_LEFT) {
            gotoxy (x1, _y);
            printf ("%c", special_chars[bc_vertical]);
        }

        if (_borders & BRD_RIGHT) {
            gotoxy (x2, _y);
            printf ("%c", special_chars[bc_vertical]);
        }
    }

    // Corners
    if (_borders & BRD_TOP) {
        if (_borders & BRD_LEFT) {
            gotoxy (x1, y1);
            printf ("%c", special_chars[bc_upper_left]);
        }

        if (_borders & BRD_RIGHT) {
            gotoxy (x2, y1);
            printf ("%c", special_chars[bc_upper_right]);
        }
    }

    if (_borders & BRD_BOTTOM) {
        if (_borders & BRD_LEFT) {
            gotoxy (x1, y2);
            printf ("%c", special_chars[bc_lower_left]);
        }

        if (_borders & BRD_RIGHT) {
            gotoxy (x2, y2);
            printf ("%c", special_chars[bc_lower_right]);
        }
    }
    delete[]s;
}

void Window::clear_line(int _y)
{
    // PROBLEM: if width changes!
    if (_y > (height-1)) 
        error("Window::clear_line _y > height-1\n");
    memcpy(canvas+width*_y, clearLine, width * sizeof(attrib));
}

void Window::draw_on_parent()
{
    if (parent) parent->copy(canvas, width, height, parent_x, parent_y);
}

void Window::popUp()
{
    Window *_parent = parent;
    _parent->remove(this);
    _parent->insert(this);
}

void Window::die() {
    Window *p = parent;
    delete this; 
    if (p) {
        p->deathNotify(this);
    }
}

Window* Window::find(Window* w)
{
    for(list<Window*>::iterator it = children.begin();it != children.end();it++)
        if (*it == w)
            return w;

    return NULL;
}

// Redraw a border window
void Border::redraw() {
    set_color(fg_white|bg_blue);
	
    box(0,0,width-1,height-1, BRD_TOP|BRD_LEFT|BRD_RIGHT|BRD_BOTTOM);
	
    // paint top/bottom messages
    
    if (top_message.len()) {
        gotoxy(1,0);
        printf("%-*.*s", min(strlen(top_message),width-2), min(strlen(top_message),width-2), ~top_message);
    }

    if (bottom_message.len()) {
        gotoxy(1,height-1);
        printf("%-*.*s", min(strlen(bottom_message),width-2), min(strlen(bottom_message), width-2), ~bottom_message);
    }
    
    dirty = false;
}

void Border::set_top_message(const char *s) {
    top_message = s;
    dirty = true;
}

void Border::set_bottom_message (const char *s)  {
    bottom_message = s;
    dirty = true;
}

bool ScrollableWindow::scroll()
{
    memmove (canvas, canvas+width, sizeof(*canvas) * width*(height-1));
    clear_line(height-1);
    cursor_y--;
    return true;
}

void messageBox(int wait, const char *fmt, ...)
{
    va_list va;
    char buf[MAX_MUD_BUF];
    
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);
    
    (void)new MessageBox(screen, buf, wait);
}

MessageBox::MessageBox(Window *_parent, const char *_message, int _wait)
:    Window(_parent, max(_parent->width/2, 3 + longestLine(_message)),
                    countChar(_message, '\n') + 3 + (_wait ? 0 : 1), Bordered, xy_center, xy_center), message(_message),
    creation_time(current_time), wait(_wait)
{
    char line[MAX_MUD_BUF];
    char *s;
    
    set_color(bg_blue|fg_bold|fg_white);
    clear();
    
    strcpy(line, _message);
    s = strtok(line, "\n");
    
    while (s)
    {
        if (*s == *CENTER)
        {
            cprintf("%s", s+1); // need to center
            print("\n");
        }
        else
            printf("%s\n", s);
        
        s = strtok(NULL, "\n");
    }
    
    if (!wait)
    {
        set_color(bg_blue|fg_bold|fg_green);
        print ("\n");
        cprintf ("Press ENTER to continue");
    }
}

void Window::set_top_message(const char *s) {
    if (parent)
        parent->set_top_message(s);
}

void Window::set_bottom_message(const char *s) {
    if (parent)
        parent->set_bottom_message(s);
}

void Window::resizeNotify(Window *who, int w, int h) {
    if (parent)
        parent->resizeNotify(who, w, h);
}

void Window::visibilityNotify(Window *who, bool visible) {
    if (parent)
        parent->visibilityNotify(who, visible);
}

void Window::moveNotify(Window *who, int w, int h) {
    if (parent)
        parent->moveNotify(who, w, h);
}

void Window::deathNotify(Window *who) {
    if (parent)
        parent->deathNotify(who);
}

int Window::trueX() const {
    return parent_x + (parent ? parent->trueX() : 0);
}

int Window::trueY() const {
    return parent_y + (parent ? parent->trueY() : 0);
}

ProxyWindow::ProxyWindow(Window *_parent, int _w, int _h, int _x, int _y) : Window(_parent, _w, _h, None, _x, _y),
proxy_target(NULL) {
}

Border::Border(Window *_parent, int _w, int _h, int _x, int _y) : ProxyWindow(_parent, _w, _h, _x, _y) {
}

void Border::visibilityNotify(Window *w, bool visible) {
    if (w == proxy_target) {
        show(visible);
    }
}

void Border::deathNotify(Window *who) {
    if (who == proxy_target)
        die();
}

void ProxyWindow::insert(Window *w) {
    proxy_target = w ;
    Window::insert(w);
}
