// The dialog box shown when you want to search scrollback
class ScrollbackSearch : public InputBox {
public:
    ScrollbackSearch (bool _forward) : InputBox (screen, _forward ? (char*)"Regexp search backwards in scrollback" : (char*)"Regexp search forward in scrollback",hi_search_scrollback), forward(_forward) {}
    virtual void execute(const char *s);
    
    NAME(ScrollbackSearch);
         
private:
    bool forward;
    
};
