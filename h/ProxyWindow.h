#ifndef DIRT_PROXYWINDOW_H
#define DIRT_PROXYWINDOW_H

#include "Window.h"
#include "String.h"

// A ProxyWindow forwards most requests to it to its child
// Used for a Border
class ProxyWindow : public Window {
public:
    ProxyWindow(Window *_parent, int _w, int _h, int _x, int _y);
    virtual void insert(Window*);

protected:
    Window *proxy_target; // first thing that's inserted
};

class Border : public ProxyWindow {
public:
    Border(Window *_parent, int _w, int _h, int _x, int _y);
    virtual void redraw();
    virtual void set_top_message(const char*);
    virtual void set_bottom_message(const char *);
    virtual void visibilityNotify(Window*, bool);
    virtual void deathNotify(Window*);

private:
    String top_message;
    String bottom_message;
};

#endif
