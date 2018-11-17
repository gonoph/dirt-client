#ifndef DIRT_PYTHONEMBEDDEDINTERPRETER_H
#define DIRT_PYTHONEMBEDDEDINTERPRETER_H

#include "EmbeddedInterpreter.h"
#include "Python.h"

class PythonEmbeddedInterpreter : public EmbeddedInterpreter
{
  public:
    ~PythonEmbeddedInterpreter();
    virtual bool load_file(const char*, bool suppress = false);
    virtual bool eval(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL);
    virtual bool run(const char*, const char*, const char* = NULL, char* = NULL, savedmatch* = NULL, bool = false);
    virtual bool run_quietly(const char*, const char*, const char*, char*,
                             bool suppress = true);
    virtual void *match_prepare(const char*, const char*);
    virtual void *substitute_prepare(const char*, const char*);
    virtual bool match(void*, const char*, char* const &);
    virtual void set(const char*, int);
    virtual void set(const char*, const char*);
    virtual int get_int(const char*);
    virtual char *get_string(const char*);
    virtual string name(void) { return myname; }
  
    PythonEmbeddedInterpreter();
    PyObject *get_function(const char*);
    PyObject *code_compile(const char*);
  protected:
    string   myname;
    PyObject *globals;
    PyObject *regexp_fixer;
};  
#endif
