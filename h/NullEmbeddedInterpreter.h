#ifndef NULLEMBEDDEDINTERPRETER_H_
#define NULLEMBEDDEDINTERPRETER_H_

#include "EmbeddedInterpreter.h"
#include "dirt.h"

class NullEmbeddedInterpreter : public EmbeddedInterpreter
{
  public:
    ~NullEmbeddedInterpreter() {}
    virtual bool load_file(const char*, bool suppress = false);
    virtual bool eval(const char*, const char *, const char*, char *result) { if(result) *result = '\0'; return false; }
    virtual void enable_function(const char*) {}
    virtual bool run(const char*, const char*, const char* = NULL, char* = NULL, bool& haserror = constboolfalse) { 
        report("Not compiled with PERL or PYTHON support. Read the Readme file.\n"); 
        if(haserror) haserror = false;
        return false; 
    }
    virtual bool run_quietly(const char*, const char*, const char*, char*,
                             bool suppress = true);
    virtual void *match_prepare(const char*, const char*) { return 0; }
    virtual void *substitute_prepare(const char*, const char*) { return 0; }
    virtual bool match(void*, const char*, char*&) { return false; }
    virtual void set(const char*, int) {}
    virtual void set(const char*, const char*) {}
    virtual int  get_int(const char*) { return 0; }
    virtual char *get_string(const char*) { return 0; }

    NullEmbeddedInterpreter() {}
};  
#endif
