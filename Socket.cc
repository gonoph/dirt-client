#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/telnet.h>
#include <sys/uio.h>
#include "dirt.h"
#include "Socket.h"

Socket::Socket(int _fd, struct sockaddr_in *_remote) {
    if (_fd == -1) {
        if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
            error ("socket: %m");
        long flags;
        flags = fcntl(fd, F_GETFL);
//FIXME
//        cout << "Allocated AF_INET socket with fd " << fd << endl;
    } else {
        fd = _fd;
        uint len = sizeof(struct sockaddr_in);
        getpeername(fd, (struct sockaddr*) &remote, &len);
        getsockname(fd, (struct sockaddr*) &local, &len);
    }
    
    linebuf_pos = 0;
    currentError = 0;
    waitingForConnection = false;
    listen_port = 0;

    if (_remote)
        remote = *_remote;
}

int Socket::bind(int port) {
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = INADDR_ANY;

    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on)) <0)
        return errno;
    
    if (::bind(fd, (struct sockaddr*) &sock, sizeof(sock)) < 0)
        return errno;

    if (::listen(fd, 42) < 0)
        return errno;

    listen_port = port;
    unsigned int namelen=sizeof(struct sockaddr_in);
    namelen=sizeof(struct sockaddr_in);
    getsockname(fd, (struct sockaddr *)&local, &namelen);
//BOB FIXME
//    long flags;
//    cout << "Flags for fd " << fd << " bound to port " << port << ":\n";
//    if(flags & O_CREAT) cout << "O_CREAT\n";
//    if(flags & O_EXCL) cout << "O_EXCL\n";
//    if(flags & O_NOCTTY) cout << "O_NOCTTY\n";
//    if(flags & O_TRUNC) cout << "O_TRUNC\n";
//    if(flags & O_APPEND) cout << "O_APPEND\n";
//    if(flags & O_NONBLOCK) cout << "O_NONBLOCK\n";
//    if(flags & O_NDELAY) cout << "O_NDELAY\n";
//    if(flags & O_SYNC) cout << "O_SYNC\n";

    return 0;
}


int Socket::connect(const char *hostname, int port, bool async) {

    if (async && (fcntl (fd, F_SETFL, FNDELAY) == -1))
        error ("Failed to make socket non-blocking: %m");
    
    // Check if inet_addr will accept this as a valid address
    // rather than assuming	whatever starts with a digit is an ip
    if ((remote.sin_addr.s_addr = inet_addr (hostname)) == (unsigned int)-1)  {
        struct hostent *h;
        
        if (!(h = gethostbyname (hostname)))
            return ENOENT;
        
        memcpy ((char *) &remote.sin_addr, h->h_addr, sizeof (remote.sin_addr));
    }
    
    remote.sin_port = htons (port);
    remote.sin_family = AF_INET;
    waitingForConnection = true;
    
    if (::connect (fd, (struct sockaddr *) &remote, sizeof (remote)) == 0 || errno == EINPROGRESS)
        return 0;
    else
        return errno;
}

void Socket::writeLine(const char *s) {
    int len = strlen(s);
    char *buf = outbuf.get(len + 2);
    memcpy(buf, s, len);
    buf[len] = '\r';
    buf[len+1] = '\n';
    outbuf.use(len+2);
    if(config->getOption(opt_undelim_prompt)) {
        inbuf.strncat(buf,len+2);
    }
}

void Socket::write (const char *buf, int len) {
    outbuf.strncat(buf,len);
    if(config->getOption(opt_undelim_prompt)) {
        inbuf.strncat(buf,len);
    }
}

void Socket::writeText(const char *buf) {
    outbuf.strcat(buf);
    if(config->getOption(opt_undelim_prompt)) {
        inbuf.strcat(buf);
    }
}

const char* Socket::getErrorText() {
    switch(currentError) {
    case errNoError:
        return "no error";
    case errHostNotFound:
        return "host not found";
    case errBufferOverflow:
        return "buffer overflow";
    case errEOF:
        return "connection closed by foreign host";
    default:
        return strerror(currentError);
    }
}

void Socket::unread(const char *buf, int count) {
    inbuf.unshift(buf,count);
}

int Socket::read(char *buf, int count) {
    if (currentError && inbuf.count() == 0)
        return -1;
    else {
        int n = min(count, inbuf.count());
        memcpy(buf, ~inbuf, min(count, n));
        inbuf.shift(n);
        return n;
    }
}

// Return the first line in the buffer, *if* we have a \n in there somewhere
const char* Socket::readLine() {
    char *nl = (char*)memchr(~inbuf, '\n', inbuf.count());
    if (nl) {
        StaticBuffer buf(nl-~inbuf+1); // +1 for the NUL
        memcpy(buf, ~inbuf, nl-~inbuf);
        buf[nl-~inbuf] = 0;
        inbuf.shift(nl-~inbuf+1); // +1 for the \n
        return buf;
    } else
        return NULL;
}

void Socket::check_fdset(fd_set *readset, fd_set *writeset) {
    bool call_output_sent = false;
    
    if (currentError == 0 && FD_ISSET(fd, writeset)) {
//FIXME      cout << "WRITE activity on a socket...\n";
        if (waitingForConnection) {
            waitingForConnection = false;
            unsigned int namelen=sizeof(struct sockaddr_in);
            namelen=sizeof(struct sockaddr_in);
            getpeername(fd, (struct sockaddr *)&remote, &namelen);
            getsockname(fd, (struct sockaddr *)&local, &namelen);
            connectionEstablished();
            
        }
        
        if (outbuf.count()) {
            int count = ::write(fd, ~outbuf, outbuf.count());
            if (count < 0)
                currentError = errno;
            else {
                outbuf.shift(count);
                if (outbuf.count() == 0)
                    call_output_sent = true;
            }
        }
    }
    
    if (FD_ISSET(fd, readset)) {
//FIXME        cout << "READ activity on a socket...\n";
        if (listen_port) {
            struct sockaddr_in sock;
            uint addrlen = sizeof(sockaddr_in);
            // Server socket waiting for connections
            int new_fd = accept(fd, (struct sockaddr*) &sock, &addrlen);
            if (new_fd >= 0) {
                connectionAccepted(new_fd, &sock);
            }
            
        } else {
            char *buf = inbuf.get(MAX_MUD_BUF);
            int count = ::read(fd, buf, MAX_MUD_BUF);
            if (count < 0)
                currentError = errno;
//                if(errno == 0) currentError = errEOF;
            else if (count == 0) {
                currentError = errEOF;
//                cout << "read() got an EOF...\n";
            } else
                inbuf.use(count);
// FIXME            if(fd == 8) {
//                cout << "read " << count << " bytes from socket " << fd << ", errno: ";
//                switch(errno) {
//                    case EINTR: cout << "EINTR"; break;
//                    case EAGAIN: cout << "EINTR"; break;
//                    case EIO: cout << "EINTR"; break;
//                    case EISDIR: cout << "EINTR"; break;
//                    case EBADF: cout << "EINTR"; break;
//                    case EINVAL: cout << "EINTR"; break;
//                    case EFAULT: cout << "EINTR"; break;
//                }
//                cout << "(" << errno << ")...\n";
//            }

            if (inbuf.count()) {
                inputReady();
            }
        }
    }
    
    if (currentError)
        errorEncountered(currentError);
    // Peculiar order, because we probably want to delete the socket when this is done!
    else if (call_output_sent)
        outputSent();
}

int Socket::init_fdset(fd_set *readset, fd_set *writeset) {
    if (currentError == 0) {
        FD_SET(fd, readset);
        if (outbuf.count() || waitingForConnection)
            FD_SET(fd, writeset);
        
        return fd;
    } else
        return -1;
}

Socket::~Socket() {
    // Last desparate flush?
    if (outbuf.count())
        ::write(fd, ~outbuf, outbuf.count());
    close(fd);
}
bool Socket::get_connection_stats(int &tx_queue, int &rx_queue, int &timer, int &retrans) {
    FILE *tcp;
    char buf[1000];
    
    tcp=fopen("/proc/net/tcp", "r");
    if (!tcp)
        return false;
    
    fgets(buf, 1000, tcp);
    while (fgets(buf, 1000, tcp)!=NULL) {
        int sl;
        unsigned int localip, remoteip;
        short unsigned int localport, remoteport;
        int st;
        int tx, rx;
        int tr;
        unsigned int when;
        int retr;
        
        if (sscanf(buf, "%d: %x:%hx %x:%hx %x %x:%x %d:%x %x",
                   &sl, &localip, &localport, &remoteip, &remoteport,
                   &st, &tx, &rx, &tr, &when, &retr)<11)
            continue;
        
        if (st!=1 ||
            local.sin_addr.s_addr != localip ||
            local.sin_port != htons(localport) ||
            remote.sin_addr.s_addr != remoteip ||
            remote.sin_port != htons(remoteport))
            continue;
        
        tx_queue=tx;
        rx_queue=rx;
        timer=tr ? when : 0;
        retrans=retr;
        
        fclose(tcp);
        return true;
    }
    
    fclose(tcp);
    return false;
}

int Socket::getLocalPort() const {
    return ntohs(local.sin_port);
}

int Socket::getRemotePort() const {
    return ntohs(remote.sin_port);
}

int Socket::writeUnbuffered(const char *s, int len) {
    int res;
    do {
        res = ::write(fd, s, len);
    } while (res < 0 && errno == EINTR);
    return res;
}

const char* Socket::getRemoteIP() const {
    return inet_ntoa(remote.sin_addr);
}

const char* Socket::getLocalIP() const {
    return inet_ntoa(local.sin_addr);
}


