// The command interpreter holds commands to be executed

#ifndef __INTERPRETER_H_
#define __INTERPRETER_H_

#include <deque>

class Interpreter {
public:
    Interpreter();
    void add(string& s) { commands.push_back(s); };
    void add(const char* s) { string str(s); add(str); };
    void execute();
    void dirtCommand (const char *command);
    void setCommandCharacter (int c);
    char getCommandCharacter();
    static bool expandSemicolon(string&);
    static bool expandSpeedwalk(string&);
    static bool command_quit(string&, void*);       // Hook callbacks for Dirt commands
    static bool command_echo(string&, void*);
    static bool command_status(string&, void*);
    static bool command_bell(string&, void*);
    static bool command_exec(string&, void*);
    static bool command_clear(string&, void*);
    static bool command_prompt(string&, void*);
    static bool command_print(string&, void*);
    static bool command_close(string&, void*);
    static bool command_open(string&, void*);
    static bool command_reopen(string&, void*);
    static bool command_send(string&, void*);
    static bool command_run(string&, void*);
    static bool command_eval(string&, void*);
    static bool command_setinput(string&, void*);
    static bool command_save(string&, void*);
private:
    deque<string> commands;
    char commandCharacter;
};

extern Interpreter interpreter;

extern bool macros_disabled, aliases_disabled, actions_disabled;

#endif
