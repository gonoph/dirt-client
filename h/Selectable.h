#ifndef DIRT_SELECTABLE_H
#define DIRT_SELECTABLE_H

#include <sys/types.h>
#include <vector>

using namespace std;

// Selectable objects are objects that need to select() on
// some FDs

// Pipe, Shell, Socket, TTY are all children of Selectable.

class Selectable {
public:
    virtual int init_fdset(fd_set *readset, fd_set *writeset) = 0;
    virtual void check_fdset(fd_set *readset, fd_set *writeset) = 0;
    Selectable();
    virtual ~Selectable();

    static void select(int, int);

private:
    static vector<Selectable*> ioList;
};

#endif
