// A shell is a Window that is executing some program and showing the results
class Shell : public ScrollableWindow, public Selectable, public Numbered
{
public:
    Shell (Window *_parent, const char *_command, int _w, int _h, int _x, int _y, int _timeout);

    virtual ~Shell() {
        killProcess();
    }

    virtual int init_fdset(fd_set *readset, fd_set *) {
        if (fd < 0)
            return 0;
        else {
            FD_SET(fd, readset);
            return fd;
        }
    }

    virtual void check_fdset(fd_set* readset, fd_set* writeset);
    virtual void idle();

    // When asked to pop up selected by alt-X, update last_input
    virtual void popUp();
    
private:

    void killProcess(); // finish off our child
    
    String command;
    pid_t pid;
    int fd;
    int timeout;       // How much time will there go since last input before we hide the window?
    time_t last_input; // When did we last hear from the window?
};
