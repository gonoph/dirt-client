typedef void SelectionCallback (void *data, const char *s, int n);

// This is a window which contains some strings. The user can
// select one of them
class Selection : public Window
{
public:
    Selection(Window *_parent, int _w, int _h, int _x, int _y);
    ~Selection();
    
    void add_string (const char *s, int color = 0);
    void prepend_string (const char *s, int color = 0);
    
protected:
    
    virtual bool keypress(int key);
    virtual const char *getData(int no);

    virtual void doSelect(int no);          // Selection bar moved over this element
    virtual void doChoose(int no, int key); // This element selected

    
    int getSelection() const { return selection; }
    void setSelection(int n) { selection = min(n,count); }
        
    int getCount() const { return count; }
    void setCount (int);                       
    virtual void redraw();
    
    int count;                          // Total number of elements
    int selection;                      // Currently selected element
    
private:

    List<char*> data;
    List<int> colors;
};

class MUDSelection : public Selection {
public:
    MUDSelection(Window *_parent);
    
private:
    virtual const char * getData(int no);
    virtual void doSelect (int no);
    virtual void doChoose (int no, int key);
    virtual bool keypress(int key);
};

