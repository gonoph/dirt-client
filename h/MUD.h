#ifndef _MUD_H_
#define _MUD_H_

#include <dirt.h>

class Action;
class Alias;
class Macro;

class MUD {
public:
    
    String name;
    
    String program;                    // Program to run to connect
    
    String commands;
    String comment;
    
    MUD *inherits; // search in this MUD if we can't find it here
    
    hash_map<string, Alias* /*, hash<string>  */> alias_list;   // Aliases unique to this MUD
    List<Action*> action_list; // Actions to be performed on output form this mud
    List<Macro*> macro_list;
    
    bool loaded;              // have we connected once? then perl stuff for this is loaded
    
    void write (FILE *fp, bool global); // Write to file. if global==true, write only aliases/actions/macros
    
    // Recursively find/check something
    Alias *findAlias(const char *name, bool recurse = true);
    Macro *findMacro(int key, bool recurse = true);
    void checkActionMatch(const char *s);
    void checkReplacement(char *buf, int& len, char **new_out);
    
    const char *getHostname() const;
    int getPort() const;
    const char *getFullName() const;
    void setHost(const char*, int);
    
    
    MUD(const char *_name, const char *_hostname, int _port, MUD *_inherits, const char *_commands = "");
    
private:
    String hostname;                   // hostname/port if using network connection
    int  port;
    
};

class MUDList : public List<MUD*> {
public:
    MUD *find (const char *_name);
};

// This MUD contains the global definitions
extern MUD globalMUD;


#endif // _MUD_H_
