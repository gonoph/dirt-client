#include "dirt.h"
#include "Interpreter.h"
#include "cui.h"
#include "Alias.h"


Selection::Selection(Window *_parent, int _w, int _h, int _x, int _y)
: Window (_parent, _w, _h, Bordered, _x, _y), count(0), selection(-1) {
    parent->focus(this);
}

Selection::~Selection() {
    for (char *s = data.rewind(); s; s = data.next())  {
		data.remove(s);
		free(s);
	}
}

void Selection::add_string (const char *s, int color) {
    data.insert(strdup(s));
    colors.insert(color);
    
	if (count++ == selection+1) // move selection down if we had the last one
		doSelect(++selection);		// selected
}

void Selection::prepend_string (const char *s, int color) {
    data.append(strdup(s));
    colors.append(color);
    
	if (count++ == selection+1) // move selection down if we had the last one
		doSelect(++selection);		// selected
}

const char * Selection::getData(int i) {
	return data[i];
}

void Selection::redraw() {
	const char *data;
	
	set_color(bg_blue|fg_white);
	clear();

	if (!children.empty())
		inputLine->redraw(); // aack. how else do we reset this f** cursor?
	
	int top = max(0, selection - height/2);
	top = min(max(0,count-height), top);
	
	for (int _y = 0; _y < height && (_y+top) < count; _y++) {
		gotoxy(0,_y);
		if ( _y+top == selection) // this is the selected line, use another color
			set_color(bg_green|fg_black);
        else {
            int color = colors[_y+top];
            if (color)
                set_color(color);
            else
                set_color(bg_blue|fg_white);
        }
		
		printf ("%-*.*s", width, width, (data = getData(_y+top)) ? data : "");
	}
	
	dirty = false;
}

bool Selection::keypress(int key) {
    dirty = true;
    
    if (Window::keypress(key))
        return true;
    
    if (selection >= 0)  {
        if (key == key_arrow_up)
            doSelect(selection = max(0,selection-1));
        
        else if (key == key_arrow_down)
            doSelect(selection = min(selection+1,count-1));
        
        else if (key == key_page_up)
            doSelect(selection = max(0, selection - height/2));
        
        else if (key == key_page_down)
            doSelect(selection = min(selection + height/2 , count -1));
        
        else if (key == key_home)
            doSelect(selection = 0);
        
        else if (key == key_end)
            doSelect(selection = count-1);
        
        else if (key >= ' ' && key < 128) { // jump to the first string with that letter
            int i, start;
            
            if (count == 0)
                status->setf("There is no data!");
            else
            {
                
                if (*getData(selection) == key)
                    start = selection+1;
                else	
                    start = 0;
                
                for (i = start; i-start < count; i++)
                    if (*getData(i%count) == key)
                        break;
                
                if ((i-start) == count)
                    status->setf ("Nothing in list that starts with '%c'", key);
                else
                    selection = i%count;
            }
        }
        
        else if (key == '\n' || key == '\r') {
            doChoose(selection, key);
        }
        else if (key == key_arrow_right) {
            doChoose(selection, key);
        }
        else if (key == key_escape)
            die();
        else
            return false;
        
    } else {
        if (key == '\n' || key == '\r')  {
            die();
        }
        else if (key == key_escape)
            die();
        else if (key == key_arrow_right || key == key_arrow_left || key == key_page_up || key == key_arrow_up || key == key_arrow_down
                 || key == key_page_down || key == key_home || key == key_end
                 || (key >= ' ' && key < 128))
            status->setf("There is no data in this list!");
        else
            return false;
    }
    
    return true;
}

void Selection::doSelect (int) {
}

void Selection::doChoose(int, int) {
    die();
}

void Selection::setCount(int no) {
    count = no;
    if (selection >= count)
        selection = count-1;
    else if (selection == -1 && count > 0)
        selection = 0;
    
    force_update();
}


void MUDSelection::doChoose(int no, int) {
    MUD *m = (*config->mud_list)[no];
    if (m)
        interpreter.add(Sprintf("%copen %s", CMDCHAR, ~m->name));
    die();
}

MUDSelection::MUDSelection (Window *_parent)
: Selection (_parent, _parent->width, _parent->height/2, 0, _parent->height/4)
{
    parent->focus(this);
    setCount(config->mud_list->count());
	doSelect(0);
    set_top_message("Mud Selector");
}

const char * MUDSelection::getData(int no) {
	static char buf[256];
	MUD *mud = (*config->mud_list)[no];

    if (mud) {
        if (strlen(mud->getHostname()))
            snprintf (buf, min(width,256), "%-20s %-20s %4d %s", ~mud->name, mud->getHostname(), mud->getPort(), ~mud->commands);
        else
            snprintf (buf, min(width,256), "%s", ~mud->name);
        return buf;
    } else
        return "";
}

void MUDSelection::doSelect (int no) {
  	char buf[128];

    if ((*config->mud_list)[no]) {
        sprintf (buf, "This mud's name: %s", ~(*config->mud_list)[no]->name);
        set_bottom_message (buf);
    }
}

bool MUDSelection::keypress(int key) {
	if ((key == 'a' || key == key_alt_a) && getSelection() < getCount()) {
		char buf[256];
		MUD *mud = (*config->mud_list)[getSelection()];
		snprintf(buf,256,"Aliases for MUD %s (%s %d)", ~mud->name, mud->getHostname(),mud->getPort());
		(void)new AliasSelection(screen, mud, buf);
	}
	else if (key == key_alt_o)
		status->setf("It's already open!");
	else
		return Selection::keypress(key);
	
	return true;
}
