/* Variables.h
 * Bob McElrath 6/4/2001
 *
 * Implementation of a hash class for variables.  (i.e. the /set command)
 * These should be accessable from scripting languages via swig.
 * They should be tie()'ed to a hash in perl.
 *
 */

#ifndef DIRT_VARIABLES_H
#define DIRT_VARIABLES_H


#include <string>
#include <hash_map>

class Variables : public hash_map<string, string, hash<string> > {
/*
    public:
    TIEHASH
    FETCH
    STORE
    DELETE
    CLEAR
    EXISTS
    FIRSTKEY
    NEXTKEY
    DESTROY
*/
    private:
};

#endif
