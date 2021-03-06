#include "WindowGroup.h"
#include "InputLine.h"
#include "Label.h"
#include "TTY.h"
#include "MUD.h"

// WindowGroup - a specialized window where one of the children at the time
// is focused - the focused child receives keypresses first, even though
// it is not the first on the list.


WindowGroup::WindowGroup(Window *_parent, int _w, int _h, int _x, int _y)
: Window (_parent, _w, _h, Bordered, _x, _y)
{
}


// Redraw
void WindowGroup::redraw()
{
	set_color(bg_blue|fg_white);
	clear();
	dirty = false;
}

void WindowGroup::insert (Window *window)
{
	focused = window;
    Window::insert(window);
}

void WindowGroup::remove (Window *window)
{
    if (window == focused) {
        for(list<Window*>::iterator it = children.begin();it != children.end();it++) {
            if(*it == window) {
                if(it != children.end()) focused = *(++it);
                else if(it != children.begin()) focused = *(--it);
                else focused = NULL;
                break;
            }
        }
    }
//        focused = focused->prev ? focused->prev : focused->next;
    
    Window::remove(window);
}

bool WindowGroup::keypress (int key)
{
    // check for tabstop - select next focused
    
    if (key == key_tab)
    {
        focused->redraw();
        
        // Does this do anything?
        for(list<Window*>::iterator it = focused->parent->children.begin();it != focused->parent->children.end();it++)
            if(*it == this) {
                if(it != focused->parent->children.end()) focused = *(--it);
                else focused = NULL;
                break;
            }
//		if (!(focused = focused->prev))
//			focused = child_last;
                
        focused->redraw();
        
        return true;
    }
    else if (key == '\r' || key == '\n')
    {
        if (!validate(val_ok))
                return true;

        finish();		
        die();
        return true;
    }
    else if (key == key_escape)
    {
        if (!validate(val_quit))
                return true;

        die();
        return true;		
    }
    
    // Ask the focused FIRST
    
    if (focused->keypress(key))
        return true;
    
    for(list<Window*>::iterator it = focused->parent->children.begin();it != focused->parent->children.end();it++) {
        if ((*it) != focused && (*it)->keypress(key))
            return true;
    }
    
    return false;
    
}

MUDEdit::MUDEdit(Window *_parent, MUD *_mud)
:  WindowGroup(_parent, 50, 7, 15, 2), mud(_mud)
{
    mud_commands 	= new InputLine(this, 40,1, None, 10,3, hi_generic);
    mud_port 		= new InputLine(this, 8,1,  None, 10,2, hi_generic);
    mud_hostname 	= new InputLine(this, 30,1, None, 10,1, hi_generic);
    mud_name 		= new InputLine(this, 10,1, None, 10,0, hi_generic);

    mud_name->set_prompt("");
    mud_hostname->set_prompt("");
    mud_commands->set_prompt("");
    mud_port->set_prompt("");
    
    mud_name->set(mud->getName());
    mud_hostname->set(mud->getHostname());
    mud_commands->set(mud->getCommands());

    char buf[16];
    sprintf(buf, "%d", mud->getPort());
    mud_port->set(buf);

    (void)new Label(this, "MUD name", 0, 1, 0, 0);
    (void)new Label(this, "Hostname", 0, 1, 0, 1);
    (void)new Label(this, "Port",     0, 1, 0, 2);
    (void)new Label(this, "Commands", 0, 1, 0, 3);
}

bool MUDEdit::validate (val_type)
{
    return true;
}

// Update the MUD edited
void MUDEdit::finish()
{
    char buf[256];
    String hostname;

    mud_name->getline(buf,true);
    mud->setName(buf);

    mud_hostname->getline(buf,true);
    hostname = buf;

    mud_commands->getline(buf,true);
    mud->setCommands(buf);

    mud_port->getline(buf,true);
    mud->setHost(hostname, atoi(buf));
}

void MUDEdit::redraw()
{
    set_color (bg_blue|fg_white);
    clear();
    dirty = false;
}
