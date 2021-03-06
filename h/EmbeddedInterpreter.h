#ifndef DIRT_EMBEDDEDINTERPRETER_H_
#define DIRT_EMBEDDEDINTERPRETER_H_

#include <stdio.h>
#include <string>
#include <vector>
#include "Option.h"
#include "Hook.h"

bool load_shared_object(const char *filename);
//FIXME unused? extern vector<string> modules_loaded;

class EmbeddedInterpreter
{
  public:
    virtual ~EmbeddedInterpreter() {}
    virtual bool load_file(const char*, bool suppress = false) = 0;
    virtual bool eval(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL) = 0;
    virtual bool run(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL, bool& = constboolfalse) = 0; 
    virtual bool run_quietly(const char*, const char*, const char*, char*,
                              bool suppres = true) = 0;
    virtual void *match_prepare(const char*, const char*) = 0;
    virtual void *substitute_prepare(const char*, const char*) = 0;
    virtual bool match(void*, const char*, char* const &) = 0;
    virtual void set(const char*, int) = 0;
    virtual void set(const char*, const char*) = 0;
    virtual int  get_int(const char*) = 0;
    virtual char *get_string(const char*) = 0;
    virtual string name(void) { return ""; }

    virtual bool isStacked() { return false; }

    static void runCallouts();

    static bool command_eval(string&,void*,savedmatch*);
    static bool command_load(string&,void*,savedmatch*);
    static bool command_run(string&,void*,savedmatch*);

protected:
    const char *findFile(const char *fname, const char *suffix); // given e.g. "foobar" and a suffix, search through the script paths
    static bool constboolfalse;

};

extern EmbeddedInterpreter *embed_interp;

class StackedInterpreter : public EmbeddedInterpreter {
public:
    StackedInterpreter(EmbeddedInterpreter *i1, EmbeddedInterpreter *i2);
    ~StackedInterpreter();
    void add(EmbeddedInterpreter *e);

    virtual bool load_file(const char*, bool suppress = false);
    virtual bool eval(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL);
    virtual bool run(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL, bool& = constboolfalse);
    virtual bool run_quietly(const char*, const char*, const char*, char*, bool suppres = true);
    virtual void *match_prepare(const char*, const char*);
    virtual void *substitute_prepare(const char*, const char*) ;
    virtual bool match(void*, const char*, char* const &) ;
    virtual void set(const char*, int);
    virtual void set(const char*, const char*);
    virtual int  get_int(const char*);
    virtual char *get_string(const char*);
    virtual string name(void) { return ""; }
        
    virtual bool isStacked() { return true; }
private:
    hash_map<string,EmbeddedInterpreter*,hashstring_type> interpreters;

};

#endif
