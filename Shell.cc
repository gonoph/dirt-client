#include "dirt.h"
#include "Shell.h"

#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


void Shell::killProcess() {
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
    
    if (pid > 0) {
        if (kill(pid, SIGHUP) >= 0) {
            usleep(300000); // wait for just a short while, hopefully the process will die off
            // In the case of the spawned java, this does NOT work
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
        }

        pid = -1;
    }
}

void Shell::check_fdset(fd_set *fdset, fd_set*) {
    if (fd >= 0 && FD_ISSET(fd, fdset))
    {
//        cout << "Activity in a shell...\n";

        char buf[1024];
        int res;

        while((res = read(fd, buf, sizeof(buf)-1) ) < 0 && errno == EINTR)
            ;

        show(true);
        last_input = current_time;
        if (res == 0)
        {
            sprintf(buf, "(Finished) %s", (const char*) command);
            set_top_message(buf);
            killProcess();
        }
        else if (res < 0)
        {
            printf("\n*** Error %m while reading.\n");
            killProcess();
        }
        else
        {
            buf[res] = NUL;
            print(buf);
        }
    }
}

Shell::Shell (Window *_parent, const char *_command, int _w, int _h, int _x, int _y, int _timeout)
:   ScrollableWindow(_parent, _w, _h, Bordered, _x, _y),  Numbered(this), fd (-1), timeout(_timeout)
{
    char buf[256];
    command = _command;
    if (number == -1)
        set_top_message(_command);
    else
    {
        sprintf(buf, "[%d] %s", number, _command);
        set_top_message(buf);
    }
    
    last_input = current_time;
    
    int pipe_fds[2];
    
    if (pipe(pipe_fds) < 0)
    {
        printf("Error while piping: %m\n");
        fd = -1;
    }
    else
    {
        fd = pipe_fds[0];
        
        pid = fork();
        
        if (pid < 0)
        {
            printf("Error while forking: %m\n");
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            fd = -1;
        }
        else if (pid == 0)
        {
            for (int i = 0; i < 255; i++)
                if (i != pipe_fds[1])
                    close (i);
            
            open("/dev/null", O_RDONLY); // -> STDIN_FILENO
            
            dup2(pipe_fds[1], STDOUT_FILENO);
            dup2(pipe_fds[1], STDERR_FILENO);
            close(pipe_fds[2]);
            execlp("/bin/sh", "/bin/sh", "-c", (const char*) command, (char*)NULL);
            _exit(1);
        }
        else
        {
            close(pipe_fds[1]);
        }
    }
};

void Shell::idle() {
    if (current_time > last_input + timeout)
    {
        if (fd > 0)
            show(false);
        else
            die();
    }
}

void Shell::popUp()
{
    last_input = current_time;
    Window::popUp();
}
