#ifndef DIRT_MESSAGEWINDOW_H
#define DIRT_MESSAGEWINDOW_H

#include "Window.h"
#include "Numbered.h"
#include <string>

// This is a small derivative of Window that is numbered and
// can be written to and created by the user and can also dump output
// coming into it to the log file

// FIXME need consistency here...implementation in MessageWindow.cc
class MessageWindow: public ScrollableWindow, public Numbered {
public:
    MessageWindow(Window *_parent, string& _alias, string& _logfile, int _w, 
            int _h, int _x, int _y, Style _style, int _timeout, bool _popOnInput, int _color);
    
    static vector<MessageWindow*> list;
    static MessageWindow* find(const string s);
    
    void addInput(const char *s);
    virtual void idle();
    virtual void popUp();
    static bool command_window(string&, void*, savedmatch*);
    ~MessageWindow();
    
protected:
    string alias;       // What is the window called?
    string logfile;     // Name of logfile to log to (if not empty)
    bool popOnInput;    // Should the window show itself when new input arrives?
    int timeout;        // How long before hiding the window?
    time_t last_input;  // When did we last get some input from this Window?
    int default_color;  // set to that before clear etc.
};

#endif
