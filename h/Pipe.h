#ifndef DIRT_PIPE_H
#define DIRT_PIPE_H


// Wrapper for pipe()
class Pipe : public Selectable {
public:
    // Create a pipe. If fd1/2 are non-NULL, the pipe is dup2'ed there
    Pipe(int fd1 = -1, int fd2 = -1);
    ~Pipe();

    int read(char *buf, int count);
    int readLine(char *buf, int count);
    int write(const char *buf, int count);

    virtual void check_fdset(fd_set *, fd_set*);
    virtual int init_fdset(fd_set *, fd_set*);

    virtual void inputReady() = 0;

    typedef enum {Read, Write} End;

    int getFile(End e);
        

private:
    int fds[2];
};


class InterpreterPipe : public Pipe {
public:
    InterpreterPipe();
    virtual void inputReady();

private:
    char line_buf[4096];
    int pos;
};

class OutputPipe : public Pipe {
public:
    OutputPipe();
    virtual void inputReady();
};


extern InterpreterPipe *interpreterPipe;
extern OutputPipe *outputPipe;

#endif
