// A buffered socket
#ifndef __SOCKET_H
#define __SOCKET_H

#include <netinet/in.h>

class Socket : public Selectable {
public:
    Socket(int sock=-1, struct sockaddr_in *remote = NULL);         // Create socket self
    ~Socket();

    enum { errHostNotFound = 1000, errBufferOverflow, errEOF, errNoError = 0};

    int connect (const char *hostname, int port, bool async);
    int bind (int port); // bind+listen
    
    // IO
    int read (char *buf, int count);               // Return number of bytes actually available
    void unread(char *buf, int count);             // Return that many bytes
    
    void write (const char *buf, int count);       // Both accept NULs in the stream

    void writeText(const char *buf);               // Let Socket find out the length
    void writeLine (const char *buf);              // string, let Socket end it with \r\n

    int writeUnbuffered(const char *buf, int len); // Write directly

    int getError();                                // For buffered writing we don't know the error until later
    const char *getErrorText();                    // Returns the errno text or our error text

    const char* readLine();   // Read line, returns pointer to static buffer

    // Selectable
    virtual int init_fdset(fd_set *readset, fd_set *writeset);
    virtual void check_fdset(fd_set *, fd_set*);

    int getFD() const { return fd; }
    bool get_connection_stats(int &tx_queue, int &rx_queue, int &timer, int &retrans);

    int getLocalPort() const;
    const char *getLocalIP() const;
    const char *getRemoteIP() const;
    int getRemotePort() const;
    int pendingInput() const { return inbuf.count(); }

    // Virtual functions for the user of Socket to override
    virtual void connectionEstablished() {};
    virtual void inputReady() {};
    virtual void errorEncountered(int) {};
    virtual void connectionAccepted(int, struct sockaddr_in*) {}
    virtual void outputSent() {} // All output buffers have been flushed

private:
    char linebuf[MAX_MUD_BUF];  // Holds partial lines
    int linebuf_pos;
    bool waitingForConnection;   // async connection in progress?
    int listen_port;

    Buffer inbuf;
    Buffer outbuf;              // output buffer, flushed when buffer is ready for writing
    int fd;
    struct sockaddr_in local;
    struct sockaddr_in remote;

    int currentError; // errno or one of the enums above
};

#endif
