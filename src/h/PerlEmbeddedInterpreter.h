#ifndef DIRT_PERLEMBEDDEDINTERPRETER_H
#define DIRT_PERLEMBEDDEDINTERPRETER_H

#include "EmbeddedInterpreter.h"
#include <EXTERN.h>
#include <perl.h>

class PerlEmbeddedInterpreter : public EmbeddedInterpreter
{
  public:
    ~PerlEmbeddedInterpreter();
    virtual bool load_file(const char*, bool suppress = false);
    virtual bool eval(const char*, const char*, const char* =NULL, char* = NULL, savedmatch* = NULL);
    virtual bool run(const char*, const char*, const char* =NULL, char* = NULL, savedmatch* = NULL, bool = false);
    virtual bool run_quietly(const char*, const char*, const char*, char*,
                             bool suppress = true);
    virtual void *match_prepare(const char*, const char*);
    virtual void *substitute_prepare(const char*, const char*);
    virtual bool match(void*, const char*, char* const &);
    virtual void set(const char*, int);
    virtual void set(const char*, const char*);
    virtual int  get_int(const char*);
    virtual char *get_string(const char*);
    virtual string name(void) { return myname; }
  
    PerlEmbeddedInterpreter();
  protected:  
    const string myname;
    PerlInterpreter *perl_interp;
    SV *default_var;
    SV *include;   
};  

//class HookStubPerl : public HookStub {
//  public:
    // Given a string, eval it when hook is triggered.
//    HookStubPerl(int p, float c, int n, bool F, string toeval) : HookStub(p, c, n, F) //...
//    virtual void operator() (char* data);
//};


#endif
