#include "Selectable.h"

#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <cerrno>
#include <cstdio>

vector<Selectable*> Selectable::ioList;

// Do a select on all registered Selectable objects
void Selectable::select(int sec, int usec)
{
    fd_set in_set, out_set;
    unsigned int i;
    int max_fd = 0;
    struct timeval tv;


    tv.tv_usec = usec;
    tv.tv_sec = sec;
    
    FD_ZERO(&in_set);
    FD_ZERO(&out_set);
    
    // Find the largest fd in the list.
    for (i = 0; ioList[i] && i < ioList.size();  i++) {
        max_fd = max(max_fd, ioList[i]->init_fdset(&in_set, &out_set));
    }
    
    // select() on it.
//    while (
    if(::select(max_fd+1, &in_set, &out_set, NULL, &tv) < 0)
        if (errno != EAGAIN && errno != EINTR)
        {
            perror ("select");
            exit (1);
        }
//    cout << "select() for " << tv.tv_sec << "s, " << tv.tv_usec << "us" << endl;

    // @@ Async connections are ready when ready to be written to, not read from
    for (i = 0; ioList[i] && i < ioList.size();  i++)
        ioList[i]->check_fdset(&in_set, &out_set);
}

Selectable::Selectable() {
    ioList.push_back(this);
}

Selectable::~Selectable() {
    for(vector<Selectable*>::iterator it = ioList.begin(); it != ioList.end(); it++) {
        if(*it == this) {
            ioList.erase(it);
            break;  // iterator is invalidated.
        }
    }
}


