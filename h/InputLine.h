#define MAX_PROMPT_BUF 80


// History ids
typedef enum {hi_none, hi_generic, hi_main_input, hi_open_mud, hi_search_scrollback} history_id;
class InputHistorySelection;

// Input line. This one handles displaying/editing
class InputLine : public Window {
public:
    InputLine(Window *_parent, int _w, int _h, Style _style, int _x, int _y, history_id _id);
    virtual ~InputLine() {};
    
    bool getline(char *buf, bool fForce); // Return true if there is a line available
    inline int  getlastkey() { return lastkey; } // return the last key pressed.
    void set_prompt (const char *prompt);
    void set_default_prompt();
    virtual void redraw();
    virtual void set (const char *s); // Set the input line to this
    virtual char* get () { return input_buf; }
    NAME(InputLine);   
protected:
    
    virtual bool keypress(int key);
    virtual void execute() {}; // Called when data has been inputted
    
protected:
    
    char input_buf[MAX_INPUT_BUF];
    char prompt_buf[MAX_PROMPT_BUF];
    int cursor_pos;     // Where will the next character be inserted?
    int max_pos;	// How many characters are there in the buffer?
    int left_pos;	// What is the left position?
    
    bool ready;		// The input line holds a finished string (hmm)
    
    history_id id;	// ID for history saving
    int history_pos;    // For cycling through history
    int lastkey;        // the last key pressed, so KeypressHookStub can get at it.
    
    
    virtual void adjust();	// Adjust left_pos
    virtual bool isExpandable() {return false;}
    
    static void selection_callback(void *obj, const char *s, int no);
    
    friend class InputHistorySelection;
};

// This is dirt's main input line
class MainInputLine : public InputLine {
public:
    MainInputLine();
    
    virtual void execute();
    virtual void set (const char *s); // Set the input line to this
    virtual bool isExpandable() { return true; }
    NAME(MainInputLine);
};

extern MainInputLine *inputLine;
