// This is a small derivative of Window that is numbered and
// can be written to and created by the user and can also dump output
// coming into it to the log file

#include "MessageWindow.h"
#include "Option.h"
#include "Config.h"
#include "Interpreter.h"
#include "Shell.h"
#include "Chat.h"
#include "Session.h"
#include "Color.h"
#include "MUD.h"
#include "Hook.h"
#include "Screen.h"
#include "StatusLine.h"

#include <vector>

extern time_t current_time;


// Does this duplicate the Window::prev and Window::next pointers?
vector<MessageWindow*> MessageWindow::list;

MessageWindow::MessageWindow (Window *_parent, string& _alias, string& _logfile, int _w, int _h, int _x, int _y, Style _style, int _timeout, bool _popOnInput, int _color)
:   ScrollableWindow(_parent, _w, _h, _style, _x, _y), Numbered(this),  alias(_alias), logfile(_logfile), popOnInput(_popOnInput), timeout(_timeout), default_color(_color)
{
    name = alias; // FIXME debug
    char buf[256];
    if (number == -1)
        buf[0] = NUL;
    else
        sprintf(buf, "[%d] ", number);

    sprintf(buf+strlen(buf), "%s ", _alias.c_str());
    if (_logfile.length())
        sprintf(buf+strlen(buf), " => %s", _logfile.c_str());
    
    if((_style & Bordered) && ((int)strlen(buf) < _w+2)) {
        set_top_message(buf);
    }

    // Make sure the blank line is the right color.
    for (attrib *a = clearLine; a < clearLine+width; a++)
        *a =  (default_color << 8) + ' ';
    
    last_input = current_time;
    list.push_back(this);
    set_color(default_color);  // FIXME this is dumb, why keep two copies of this?
    clear();
    name = alias; // FIXME debug
}

void MessageWindow::addInput(const char *s)
{
    if (popOnInput)
    {
        show(true);
        popUp();
    }
    
    printf("%s", s);
    if (logfile.length())
    {
        FILE *fp = fopen(logfile.c_str(), "a");
        if (fp)
        {
            fprintf(fp, "%s", s);
            fclose(fp);
        }
    }
}

void MessageWindow::idle()
{
    if (visible && timeout > 0 && last_input+timeout < current_time)
        show(false);
}

void MessageWindow::popUp()
{
    last_input = current_time;
    Window::popUp();
}

bool MessageWindow::command_window(string& str, void*, savedmatch*) {
    int w = 80, h=10, x=0, y=3, t=10;
    bool popup = true;
    bool noborder = false;
    int color = bg_black|fg_white;
    string logfile;
    string name;

//    outputWindow->printf("MessageWindow::command_window\n");
    OptionParser opt(str, "HBisklw:h:x:y:t:L:c:");
    if(!opt.valid()) return true;
    
    if(opt.gotOpt('w')) w = atoi(opt['w'].c_str());
    if(w <= 0) w = screen->width + w;
    if(opt.gotOpt('h')) h = atoi(opt['h'].c_str());
    if(h <= 0) h = screen->height + h;
    if(opt.gotOpt('x')) x = (opt['x'][0]=='-')?(screen->width-atoi(opt['x'].c_str())-w):atoi(opt['x'].c_str());
    if(opt.gotOpt('y')) y = (opt['y'][0]=='-')?(screen->height-atoi(opt['y'].c_str())-h):atoi(opt['y'].c_str());
    if(opt.gotOpt('t')) t = atoi(opt['t'].c_str());
    if(opt.gotOpt('L')) logfile = opt['L'];
    if(opt.flag('H')) popup = false;
    if(opt.flag('B')) noborder = true;
    if(opt.gotOpt('c')) color = atoi(opt['c'].c_str());
    name = opt.arg(1);
    
    int min_h = noborder?1:3;
    MessageWindow *mw;
    int CmdChar = config->getOption(opt_commandcharacter);

    if(opt.gotOpt('l')) {
        report("Windows in existence:\n");
        for(unsigned int i=0;i<list.size();i++) {
            report("\t%s\n", list[i]->alias.c_str());
        }
        return true;
    }
    if(opt.gotOpt('s')) {
        if (!(mw = MessageWindow::find(name)))
            status->setf ("%cwindow -s: No such window: %s", CmdChar, name.c_str());
        else {
            mw->show(true);
//            mw->popUp();
        }
    }
    if(opt.gotOpt('i')) {
        if (!(mw = MessageWindow::find(name)))
            status->setf ("%cwindow -s: No such window: %s", CmdChar, name.c_str());
        else {
            mw->show(false);
//            mw->popUp();
        }
    }
    // Do some sanity checking
    else if (!name.length())
        status->setf("%cwindow needs a name", CmdChar);
    else if(opt.gotOpt('k')) {
        if (!(mw = MessageWindow::find(name)))
            status->setf ("%cwindow -k: No such window: %s", CmdChar, name.c_str());
        else
            mw->die();
    }
    else if (h < min_h)
        status->setf("%cwindow: Value h=%d too low.", CmdChar, h);
    else if (w < min_h)
        status->setf("%cwindow: Value w=%d too low.", CmdChar, w);
    else if (w+abs(x) > screen->width)
        status->setf("%cwindow: Value w=%d or x=%d too large.  Window would extend beyond screen.", CmdChar, w, x);
    else if (abs(y)+h > screen->height)
        status->setf("%cwindow: Value h=%d or y=%d too large.  Window would extend beyond screen.", CmdChar, h, y);
    else if (MessageWindow::find(name))
    {
        status->setf("Window %s already exists", name.c_str());
    }
    else  {
        Window::Style style;
        if (!noborder)
            style = Window::Bordered;
        else
            style = Window::None;
        
        mw =  new MessageWindow(screen, name, logfile, w,h,x,y, style, t, popup, color);
        
        if (!popup)
            mw->show(false);
    }
    return true;
}

MessageWindow* MessageWindow::find (const string s)
{
    for (unsigned int i=0;i<list.size();i++) {
        MessageWindow *m = list[i];
        if (m->alias == s)
            return m;
    }

    return NULL;
}

MessageWindow::~MessageWindow()
{
    // vectors are O(N), but we don't expect to be deleting windows often.
    for(vector<MessageWindow*>::iterator it = list.begin();it != list.end();it++) {
        if(*it == this) {
            list.erase(it);
            break;
        }
    }

}
