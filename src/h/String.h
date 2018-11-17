#ifndef DIRT_STRING_H
#define DIRT_STRING_H

#include "defs.h"
#include "cstring"
// Really simple shared string class

class String {
private:
    void replace (const char *s) {
        // Empty strings "", are not allocated/freed
        if (*_s)
            delete _s;
        if (*s) {
            _s = new char[strlen(s)+1];
            strcpy(_s, s);
        } else {
            _s = "";
        }
            
    }

    
public:
    String() : _s("") {}
    
    String(const char *s) { _s=""; replace(s); }
    String(const char *s, int len)  {
        _s="";
        char buf[len+1];
        memcpy(buf, s, len);
        buf[len] = 0;
        replace(buf);
    }

    // Copy constructor
    String(const String& s2) { _s=""; replace(s2._s);}

    void operator= (const String& s2) {
        if (&s2 != this) // Don't free/str_dup if assigning to self
            replace(s2._s);
    }

    void operator= (const char *s2) { replace (s2); }

    ~String() { if (*_s) delete[] _s; }

    operator const char *() const {return _s; }
    int len() const { return strlen(_s); }

    int operator[] (int n) const { return _s[n]; } // unchecked?

    // Note that this is a case INSENSITIVE comparison!
    bool operator== (const char *s) const { return !strcasecmp(s,_s); }
    bool operator!= (const char *s) const { return strcasecmp(s,_s); }

    const char* operator~() const { return _s; }

    // Is this useful?
    int printf(const char *fmt, ...);

private:
    char *_s;
};

#endif

