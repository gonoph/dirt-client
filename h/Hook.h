/* Hook.h
 * 
 * 2/10/2001 Bob McElrath
 * 
 * Hooks are classes of callbacks.  In other words, you can register a function
 * to be executed when a hook is called.  The hook 'output' for instance, will
 * be called each time there is a line of output from the mud.  A function
 * registered as an 'output hook' will get called once for each line of output.
 *
 * Add new (C++) hooks like:
 *   hook.add(output, new CppHookStub(priority, chance, shots, fallthrough));
 * Note: the Hook class will delete the CppHookStub object if it is removed.
 */

#ifndef __HOOK_H
#define __HOOK_H

#include "dirt.h"

#include <set>      // stores list of HookStub's
#include <vector>   // Stores list of sets of HookStub's by priority.
#include <string>   // strings are better than char*
#include <hash_map> // for types list

template <class _Tp>
struct priority_less : public binary_function<_Tp,_Tp,bool> 
{
    // This is defined as > because we want priority-ordering to have highest
    // priorities first, and lowest priorities last.  'set' uses the < operator,
    // not the > operator.  (there's probably a better way to do this...)
    bool operator()(const _Tp& __x, const _Tp& __y) const { return (*__x)[0]->priority > (*__y)[0]->priority; }
};

typedef enum {  // These are used as indices into hooks.  They should range 0..
    COMMAND   = 0,   // command typed by user, before it's sent to mud
    USERINPUT = 1,   // called after user types 'enter'
    SEND      = 2,   // called just before text is sent to mud.
    OUTPUT    = 3,   // called on any output from mud -- use for triggers
    LOSELINK  = 4,   // called when link is lost
    PROMPT    = 5,   // called when a new prompt arrives
    INIT      = 6,   // called on startup
    DONE      = 7,   // called when dirt exits. (use to kill procs, save persistent data)
    KEYPRESS  = 8,   // called for each key pressed by user
    CONNECT   = 9,   // run when user attempts to connect to a mud.
    IDLE      = 10   // run once per second.
} HookType;

// Stub for hooks.  Embedded interpreters must subclass this and override 
// operator().  This is how Dirt calls hooks.
class HookStub { 
friend class Hook;
friend class priority_less<vector<HookStub*>* >;
protected:
    int     priority;   // larger is more important, can be negative
    float   chance;     // 0 -> 1 probability
    int     shots;      // number of times to execute this hook before deleting it.
    bool    fallthrough;// if true, other hooks of lower priority will also be executed.
    bool    enabled;
    bool    color;      // Whether we want to match against color or not.
    string  name;
    vector<string>  groups;
    HookType type;
public:                 // is defined *after* HookStub).
    HookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g);
    virtual bool operator() (string& data) = 0; // Children must override this.
    // needed to put this into an ordered container (i.e. priority_queue<int>).
    bool operator< (const HookStub& b) const { return(priority < b.priority); }
    virtual ~HookStub() {};
};

// A callback class that stores a C function pointer.
class CppHookStub : public HookStub {
    bool (*callback)(string&);
public:
    CppHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g, bool (*cbk)(string&));
    virtual bool operator() (string& data);
    virtual ~CppHookStub() {};
};

// A callback class that stores a function pointer to a static class method,
// and a pointer to an instance of the class (which must be cast to the 
// appropriate class pointer type)  The command must match "commandname".
class CommandHookStub : public HookStub {
    bool (*callback)(string&, void*);
    void *instance;
    string commandname;
public:
    CommandHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g, 
                string cmdname, bool (*cbk)(string&,void*), void* ins);
    virtual bool operator() (string& data);
    virtual ~CommandHookStub() {};
};

// This hook has two arguments: a regex, and argument.  If the regex matches,
// the argument is evaluated as a quoted string (i.e. if the argument contains
// $1, the $1 will be replaced by the first parenthetical in the regex)
class TriggerHookStub : public HookStub {
    void*   matcher;
    string  command;    // command to run on match.
    char*   language;   // command is a perl/python function.
public:
    TriggerHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g,
            string regex, string arg, const char* lang);
    virtual bool operator() (string& data);
    virtual ~TriggerHookStub() {};
};

// FIXME Will this ever need a regex, matcher, or command?  Make this a child of CppHookStub instead?
class KeypressHookStub : public TriggerHookStub {
    int key;
    string window;   // Can't store a MessageWindow* because if the window is killed, it would be
                     // pointing to unallocated memory, and we wouldn't know it.
    bool (*callback)(string&, void*);
    void *instance;
public:
    KeypressHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g,
            string regex, string arg, const char* lang, int k, string w, bool (*cbk)(string&,void*) = NULL, 
            void* ins = NULL);
    virtual bool operator() (string& data);
    virtual ~KeypressHookStub() {};
};
    

class Hook {
public:
    Hook();
    HookType add_type(const string& name);    // add a new hook type
    void add(HookType t, HookStub* callback); // Note: callback will be copied.
    void add(string name, HookStub* callback);  // Note: callback will be copied.
    void addDirtCommand(string name, bool (*cbk)(string&,void*), void* instance);
    bool remove(string name);                   // returns true if successful, false 
    bool disable(string name);                  // if there was no hook with that name.
    bool disableGroup(string group);
    bool enable(string name);
    bool enableGroup(string group);
    // FIXME the following bool return types are only needed for KEYPRESS hooks and
    // their interaction with the OLD keypress system.  They can be changed to void
    // when the old keypress system is removed.
    void run(HookType type, string& data);           // data may be modified.
    void run(HookType type, char* data = NULL);
    void run(string const type, string& data) { return run(types[type], data); };
    void run(string const type, char* data = NULL) { return run(types[type], data); };
    static bool command_hook(string&,void*);
    static bool command_disable(string&,void*);
    static bool command_enable(string&,void*);
    static bool command_group(string&,void*);
    ~Hook();        // Will destroy everything in hooks
private:
    int max_type;   // = IDLE

    // These two data structures hold the same data (and should be consistent).
    // 'hooks' is for quickly calling all hooks of a particular type.
    // 'hooknames' is for quickly manipulating the hooks by name.  Since it's
    //   a hash map, it has the added benefit of preventing two hooks with the
    //   same name from being used.
    typedef multiset<vector<HookStub*>*, priority_less<vector<HookStub*>* > > hookstubset_type;
    typedef vector<hookstubset_type*>                              hooks_type;
    typedef hash_map<string, HookStub*, hash<string> >             hooknames_type;
    typedef hash_map<string, vector<HookStub*>, hash<string> >     hookgroups_type;
    hooks_type hooks; // subscript of hooks (hooks[i]) is HookType.
    hooknames_type hooknames;
    hookgroups_type hookgroups;

    typedef hash_map<string, HookType, hash<string> > types_type;
    types_type types;
};

extern Hook hook;

#endif // __HOOK_H
