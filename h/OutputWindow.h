#ifndef DIRT_OUTPUTWINDOW_H
#define DIRT_OUTPUTWINDOW_H


class ScrollbackController;

class OutputWindow : public Window
{
public:
    OutputWindow(Window *_parent);
    virtual ~OutputWindow();
    
    void printVersion();
    NAME(OutputWindow);

    void search (const char *s, bool forward);
    virtual void move(int, int);
    void saveToFile(const char *fname, bool color); // Save scrollback to file
    ScrollbackController* getsb() { return sb; }
    
private:
    
    typedef enum { 
        move_home, move_page_up, move_line_up, move_stay, 
        move_line_down, move_page_down
    } move_t;
    
    virtual bool scroll();
    
    virtual void draw_on_parent();
    
    void freeze()   { fFrozen = true; }    // Freeze viewpoint
    void unfreeze() { fFrozen = false; viewpoint = canvas; } // Unfreeze
    
    void moveViewpoint(int amount);    // Move the viewpoint. true if reached end of buffer
    
    attrib *scrollback; // Beginning of the buffer
    attrib *viewpoint;     // What are we viewing?

    int top_line;        // What is the number of the top line in the scrollback?
    struct
    {
        int line;        // What line # to highlight?
        int x;           // What x offset to start at ?
        int len;         // How much to highlight?
    } highlight;
    
    bool    fFrozen;    // Should we move viewpoint or not?
    ScrollbackController *sb;
    friend class ScrollbackController;
};

// This is an invisible object that controls the scrollback process
class ScrollbackController : public Window
{
public:
    ScrollbackController(Window *_parent, OutputWindow *_output);
    
//    virtual bool keypress(int key);
    static bool keypress_page_up(string&, void*);
    static bool keypress_page_down(string&, void*);
    static bool keypress_arrow_up(string&, void*);
    static bool keypress_arrow_down(string&, void*);
    static bool keypress_home(string&, void*);
    static bool keypress_pause(string&, void*);
    static bool keypress_end(string&, void*);
    void close();
    NAME(ScrollbackController);
    virtual ~ScrollbackController();   
private:
    OutputWindow *output;
};

extern OutputWindow *outputWindow;

#endif
