// The message box displays a message and either
// 1) Disappears and dies after N seconds
// or 2) waits for user to press RETURN

// This class should be usually derived from, overriding the execute()
// method



#define CENTER "\001"

class MessageBox : public Window
{
public:
    // If wait == 0, wait until a key is pressed
    MessageBox(Window *_parent, const char *_message, int _wait);
    
    virtual void execute() {}
    virtual void idle()
    {
        if (wait > 0 && current_time > creation_time+wait)
        {
            execute();
            die();
        }
    }
    
    bool keypress(int key)
    {
        if (wait == 0)
        {
            if(key == '\n' || key == '\r')
            {
                execute();
                die();
            }
            return true;
        }
        
        return false;
    }
    
private:
    String message;
    time_t creation_time;
    int wait;
};

void messageBox(int wait, const char *fmt, ...);

