//
// Generic window
//

#include <deque>
#include <list>
typedef unsigned short int attrib;

enum {wh_half = -5000, wh_full = -5001}; // this sucks, plain and simple
enum {xy_center = -5002 };    // aargh! But what can we do?


#define NAME(x) virtual const char* getName() const { return #x; }

class WindowGroup;
 
class Window
{
public:
    enum Style {None=0, Bordered=1}; // styles
        
public:
    string name; // FIXME debug
    Window *parent;                    // Parent of this window

    int width,height;                // Dimensions
                                    // Create the window with this size
    Window(Window *_parent, int _width = wh_full, int _height = wh_full, Style _style = None, int _x = 0, int _y = 0);
                                    
    virtual ~Window();                // Destroy window, remove from parent's
                                    // list

    void box(int x1, int y1, int x2, int y2, int _borders);
    virtual void draw_on_parent();    // Draw yourself upon your parent!    
    virtual void redraw();             // Redraw contents of THIS window onto canvas
    virtual bool keypress(int key);    // Handle this keypress. true if taken care of
    virtual void
        set_cursor (int _x,int _y);    // Set the physical cursor (hmm.)
    bool refresh();                    // Redraw if dirty, then refresh upon parent

    virtual void show(bool);
    virtual void die();                // Suicide
    virtual void popUp();           // pop to top
    virtual void resize(int w, int h); // Resize, reallocated canvas
    virtual void move(int x, int y);  // Change parent_x/y

    int trueX() const;                // X/Y coordinates as they are on the final screen
    int trueY() const;

    // notifiers
    virtual void resizeNotify(Window *, int, int); // child has resized
    virtual void moveNotify(Window*, int, int); // child has moved
    virtual void visibilityNotify(Window *, bool); // child has been shown or hidden
    virtual void deathNotify(Window *); // child has died

    void force_update() { dirty = true; } // Redraw when refreshing no matter what
    
    void clear_line (int _y);        // Clear this line with blanks
    
    virtual void idle();            // Call own then childrens' idle method
    
    virtual void insert(Window *window);    // Insert this window in this child list
    void append(Window *window);    // Insert window, on top
    virtual void remove(Window *window);    // Remove this window from this child list
    
    void printf (const char *fmt, ...); // Wrapwrite(f) this string on current x,y
    void cprintf (const char *fmt, ...); // Print this line, centered
    void print  (const char *s);        // As above, but do not format
    void copy (attrib *source, int w, int h, int _x, int _y); // Copy this w*h rect to x,y
    virtual bool scroll();            // Scroll window. Return true if possible
    
    void set_color(int _color)        // Set color
    { color = _color; }

    void focus(Window *w) { focused = w; }
        
    void gotoxy (int _x, int _y)        // Goto these coordinates
        { cursor_x = _x; cursor_y = _y; }
    
    void clear();                    // Clear window (fill with color)
    bool is_focused();                // Are we focused?
    bool is_visible() { return visible; }
    bool is_bordered() { return(style == Bordered); }

    // Find a Border-type Window longer up and set their message
    virtual void set_top_message (const char *s);
    virtual void set_bottom_message (const char *s);

    friend class WindowGroup;                    // argh

    Window* find(Window *w); // Is this window still one of my children?

    Window* findByName(const char *name); // Find a window with this typename

    NAME(Window);

protected:
    
    list<Window*> children;

    unsigned int visible : 1;        // Is this window visible? 
    
    int color;                        // Current color

    Window *focused;
    int parent_x,parent_y;            // Position of this window in parent
    int cursor_x,cursor_y;            // Position of cursor
    attrib *canvas;                    // The window's data
    unsigned int dirty : 1;            // Do we have to redraw when we refresh?
    Style style;

    void check();                   // Integrity check

    attrib *clearLine; // clearing a line happens often
    deque<unsigned char> saved_colors;
}; 

class ScrollableWindow : public Window {
    public:
    ScrollableWindow(Window *_parent, int _w, int _h, Style _style, int _x, int _y)
    : Window(_parent, _w, _h,  _style, _x, _y) {}
    
    virtual bool scroll();
    NAME(ScrollableWindow);
};
