// The command interpreter holds commands to be executed

#ifndef DIRT_INTERPRETER_H
#define DIRT_INTERPRETER_H

#include <deque>
#include <Hook.h>

class Interpreter {
public:
    Interpreter();
    // add a command to the end of the stack.
    void add(string& s, string& data, string regex) { 
        commands.push_back(pair<string,savedmatch*>(s, new savedmatch(data, regex))); 
    };  // This gets deleted in Interpreter::execute
    void add(string& s) { commands.push_back(pair<string,savedmatch*>(s,NULL)); }; 
    void add(const char* s, string& data, string regex) { string str(s); add(str, data, regex); };
    void add(const char* s) { string str(s); add(str); }
    void add(string& s, savedmatch* sm) { 
        commands.push_back(pair<string,savedmatch*>(s, new savedmatch(*sm))); 
    }
    // insert a command on the beginning of the stack
    void insert(string& s, string& data, string regex) { 
        commands.push_front(pair<string,savedmatch*>(s, new savedmatch(data, regex))); 
    };  // This gets deleted in Interpreter::execute
    void insert(string& s) { commands.push_front(pair<string,savedmatch*>(s,NULL)); }; 
    void insert(const char* s, string& data, string regex) { 
        string str(s); insert(str, data, regex); 
    };
    void insert(const char* s) { string str(s); insert(str); }
    void insert(string& s, savedmatch* sm) { 
        commands.push_front(pair<string,savedmatch*>(s, new savedmatch(*sm))); 
    }
    void dump_stack(void);                          // dump the command stack to the screen.
    void execute();
    void dirtCommand (const char *command);
    void setCommandCharacter (int c);
    char getCommandCharacter();
    static bool expandSemicolon(string&);
    static bool expandSpeedwalk(string&);
    static bool command_quit(string&,     void*, savedmatch*);// Hook callbacks for Dirt commands
    static bool command_echo(string&,     void*, savedmatch*);
    static bool command_status(string&,   void*, savedmatch*);
    static bool command_bell(string&,     void*, savedmatch*);
    static bool command_exec(string&,     void*, savedmatch*);
    static bool command_clear(string&,    void*, savedmatch*);
    static bool command_prompt(string&,   void*, savedmatch*);
    static bool command_print(string&,    void*, savedmatch*);
    static bool command_close(string&,    void*, savedmatch*);
    static bool command_open(string&,     void*, savedmatch*);
    static bool command_reopen(string&,   void*, savedmatch*);
    static bool command_send(string&,     void*, savedmatch*);
    static bool command_run(string&,      void*, savedmatch*);
    static bool command_eval(string&,     void*, savedmatch*);
    static bool command_setinput(string&, void*, savedmatch*);
    static bool command_save(string&,     void*, savedmatch*);
private:
    deque<pair<string,savedmatch*> > commands;
    char commandCharacter;
};

extern Interpreter interpreter;

extern bool macros_disabled, aliases_disabled, actions_disabled;

#endif
