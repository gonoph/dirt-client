#include "Pipe.h"
#include "misc.h"
#include "Interpreter.h"
#include "OutputWindow.h"

#include <unistd.h>
#include <limits.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>

Pipe::Pipe(int fd1, int fd2) {
    // socketpair has apparently greater 'capacity' than pipe
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
        error ("Pipe::Pipe socketpair: %m");
    
    if (fd1 != -1) {
        if (dup2(fds[Read], fd1) < 0)
            error ("Pipe::Pipe dup2 (%d->%d) %m", fds[Read], fd1);
        close(fds[Read]);
        fds[Read] = fd1;
    }

    if (fd2 != -1) {
        if (dup2(fds[Write], fd2) < 0)
            error ("Pipe::Pipe dup2 (%d->%d) %m", fds[Write], fd2);
        close(fds[Write]);
        fds[Write] = fd2;
    }
}

Pipe::~Pipe() {
    close(fds[0]);
    close(fds[1]);
}

int Pipe::read (char *buf, int count) {
    count = ::read(fds[Read], buf, count);
    return count;
}

int Pipe::write(const char *buf, int count) {
    count = ::write(fds[Write], buf, count);
    return count;
}

int Pipe::init_fdset(fd_set *set, fd_set*) {
    FD_SET(fds[Read], set);
    return fds[Read];
}

void Pipe::check_fdset(fd_set *set, fd_set*) {
    if (FD_ISSET(fds[Read], set)) {
//FIXME        cout << "Activity on a pipe...\n";
        inputReady();
    }
}

InterpreterPipe::InterpreterPipe() : Pipe(), pos(0) {
}

void InterpreterPipe::inputReady() {
    int res;
    char *s;
    
    res = read(line_buf+pos, sizeof(line_buf)-pos);
    if (res <= 0)
        error ("inputReady::read:%m");
    
    pos += res;
    
    while ((s = (char*)memchr(line_buf, '\n', pos))) {
        char buf[PIPE_BUF];
        int len = s-line_buf;
        
        memcpy(buf, line_buf, len);
        buf[len] = NUL;
        memmove(line_buf, s+1, pos - len);
        pos -= len+1;
        interpreter.add(buf);
    }
}

bool InterpreterPipe::have_data() {
    pollfd myfd;
    myfd.fd = fds[0];  // My read fd
    myfd.events = 0;
    myfd.events |= POLLIN;
    poll(&myfd, 1, 0);
    return(myfd.revents & POLLIN);
}

OutputPipe::OutputPipe() : Pipe(-1, STDOUT_FILENO) {
}

void OutputPipe::inputReady() {
    char buf[PIPE_BUF];
    int count = read(buf, sizeof(buf));
    if (count < 0)
        error ("OutputPipe::inputReady:%m");
    
    buf[count] = NUL;
    outputWindow->printf("%s", buf);
}

int Pipe::getFile(End e) {
    return (fds[e]);
}


