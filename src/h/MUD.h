#ifndef DIRT_MUD_H
#define DIRT_MUD_H

#include "dirt.h"
#include "List.h"

#include <cstdio>

class MUD {
public:
    void write (FILE *fp, bool global); // Write to file. if global==true, write only aliases/actions/macros
    const char *getHostname() const;
    int getPort() const;
    const char *getFullName() const;
    void setHost(const char*, int);
    void setName(const char* c) { name = c; }
    const char *getName() const { return name.c_str(); }
    void setCommands(const char* c) { commands = c; }
    const char *getCommands() const { return commands.c_str(); }
    void setLoaded(bool l) { loaded = l; }
    bool getLoaded() { return loaded; }
    void setInherits(MUD* i) { inherits = i; }
    
    MUD(const char *_name, const char *_hostname, int _port, MUD *_inherits, const char *_commands = "");
    
private:
    string hostname;                   // hostname/port if using network connection
    int  port;
    string name;
    string commands;
    MUD *inherits; // search in this MUD if we can't find it here
    bool loaded;              // have we connected once? then perl stuff for this is loaded
};

class MUDList : public List<MUD*> {
public:
    MUD *find (const char *_name);
};

// This MUD contains the global definitions
extern MUD globalMUD;


#endif // _MUD_H_
