// The implementation for this is in Alias.cc

// An action (regexp) to be taken
class Action
{
public:
    ~Action();
    
    String pattern; // The pattern to search for
    String commands; // Commands to be run OR replacement pattern
    
    typedef enum {
        Trigger, Replacement, Gag
    } action_t;
    
    action_t type;
    void *embedded_sub;

    void compile();
    void checkMatch(const char*); // run commands if matching
    void checkReplacement(char *buf, int& len, char **new_out);
    
    // Take a string like "^blah" dosomething and produce an Action
    // if NULL, error ocurred:  error result will be in res
    static Action* parse(const char *buf, String& res, action_t type);
    
private:
    Action(const char *_pattern, const char *_commands, action_t type);
};
