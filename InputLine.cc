// The input line
#include "dirt.h"
#include "cui.h"
#include "Hook.h"
#include "Interpreter.h"
#include <sys/stat.h>

History::History(int _id) : id (_id), current(0) {
    max_history = config->getOption(opt_histsize);
    strings = new (char*)[max_history];
    timestamps = new time_t[max_history];
	
    // Hmm, not sure about this
    memset(strings,0, max_history * sizeof(char*));
}

History::~History() {
    delete[] strings;
    delete[] timestamps;
}

void History::add(const char *s,time_t t) {
	// Don't store multiple strings that are exactly the same
	if (current > 0 && !strcmp(s,strings[(current-1) % max_history]))
		return;
	
	if (strings[current % max_history])
		free(strings[current % max_history]);
	
    strings[current % max_history] = strdup(s);
    timestamps[current % max_history] = t;

	current++;
}

// getting number 1 gets you the LAST line
const char * History::get(int count, time_t* timestamp) {
    if (count > min(current, max_history))
        return NULL;
    
    if (timestamp)
        *timestamp = timestamps[(current - count) % max_history];
    return strings[(current - count) % max_history];
}

HistorySet::HistorySet() : hist_list(hi_search_scrollback+1) {
    for(int i=hi_none;i<=hi_search_scrollback;i++) {
        hist_list[i] = new History(i);
    }
}
HistorySet::~HistorySet()  {
    for (hist_list_t::iterator it = hist_list.begin(); it != hist_list.end(); it++) {
        delete *it;
        hist_list.erase(it);
    }
}

void HistorySet::saveHistory() {
    if (config->getOption(opt_save_history)) {
        FILE *fp = fopen(Sprintf("%s/.dirt/history", getenv("HOME")), "w");
        if (fp) {
            const char *s;
            time_t t;
            fchmod(fileno(fp), 0600);
            for(hist_list_t::iterator it = hist_list.begin(); it != hist_list.end(); it++)
                for (int i = config->getOption(opt_histsize) ; i > 0; i--)
                    if ((s = get((history_id)(*it)->id,i, &t)))
                        fprintf(fp, "%d %ld %s\n", (*it)->id, t, s);
            fclose(fp);
        }
    }
}

void HistorySet::loadHistory() {
    if (config->getOption(opt_save_history)) {
        FILE *fp = fopen(Sprintf("%s/.dirt/history", getenv("HOME")), "r");
        if (fp) {
            int id;
            time_t t;
            char buf[1024];
            
            while (3 == fscanf(fp, "%d %ld %1024[^\n]", &id, &t, buf))
                hist_list[id]->add(buf, t);
            fclose(fp);
        }
    }
}

class InputHistorySelection : public Selection{
public:
    InputHistorySelection(Window *parent, int w, int h, int x, int y, InputLine& _input, bool _enterExecutes, history_id _historyId) : Selection(parent,w,h,x,y),
        inputLine(_input), enterExecutes(_enterExecutes), historyId(_historyId) {
            const char *s;
            for (int i = 1; (s = history->get(historyId,i, NULL)); i++)
                prepend_string(s);
        }
    
    virtual void idle() {
        doSelect(getSelection());
        force_update();
    }
    
    virtual void doSelect(int no) {
        time_t t;
        char buf[64];
        assert(history->get(historyId, getCount() - no, &t));
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));

        const char *unit = "second";
        int n = current_time - t;
        if (n > 120) {
            n /= 60;
            unit = "minute";
            if (n > 120) {
                n /= 60;
                unit = "hour";
                if (n > 48) {
                    n /= 24;
                    unit = "day";
                }
            }
        }
        
        set_bottom_message(Sprintf("Executed on: %s (%d %s%s ago)", buf, n, unit, n == 1 ? "" : "s"));
    }
    
    virtual void doChoose(int no, int key) {
        const char *s = history->get(historyId, getCount() - no,NULL);
        assert(s != NULL);
        inputLine.set(s);
        inputLine.keypress(key);
        die();
    }

    virtual bool keypress(int key) { // adds editing keys as valid selectors
        dirty = true;

        if(selection >= 0 && (key == '\n' || key == '\r' || key == key_arrow_right 
            || key == key_arrow_left || key == key_ctrl_h || key == key_backspace
            || key == key_ctrl_a || key == key_ctrl_j || key == key_ctrl_k 
            || key == key_ctrl_e || key == key_ctrl_w || key == key_delete 
            || key == key_kp_enter || key == ' ' || key == key_tab)) { 
            doChoose(selection, key); 
            return true;
        } else {
          return Selection::keypress(key);
        }
    }

private:
    InputLine& inputLine; // Input line to do the changes on
    bool enterExecutes;   // Enter sends \r to the input line too
    history_id historyId;     // History to get the data from
};

InputLine::InputLine(Window *_parent, int _w, int _h, Style _style, int _x, int _y, history_id _id)
: Window(_parent, _w, _h, _style, _x, _y),
cursor_pos(0), max_pos(0), left_pos(0), ready(false), id(_id), history_pos(0)
{
	set_default_prompt();
}

void InputLine::set_default_prompt() {
	strcpy(prompt_buf, ">");
	adjust();
	dirty = true;
}

void InputLine::set(const char *s) {
	strcpy (input_buf, s);
	max_pos = strlen(input_buf);
	cursor_pos = max_pos;
    left_pos = 0;

	adjust();
	dirty = true;
}

void MainInputLine::set (const char *s) {
    InputLine::set(s);
    if (*s == NUL) { // clear, go back to standard size
        move(0, parent->height-1);
        resize(width, 1);
        outputWindow->move(0,0);
    }
}

// Handle a keypress
bool InputLine::keypress(int key) {
    lastkey = key;
    if (ready)
        return true;
    
    embed_interp->set("Key", key);
    dirty = true; // let's just assume this to make things easier
    
    int prev_len = max_pos;
    
//    report("calling hook.run(KEYPRESS, %s)", input_buf);
    hook.run(KEYPRESS, input_buf); // FIXME MOVE to TTY::check_fdset
//    report("returned from hook.run(KEYPRESS, %s)", input_buf);
    // This is tough - what do we adjust here
    int new_len = strlen(input_buf);
    max_pos += (new_len - prev_len);
    cursor_pos += (new_len - prev_len);
    cursor_pos = max(cursor_pos, 0);
    
    // set Key to 0 if keypress handled. Ugh FIXME: make all of the below hooks.
    if ((key = embed_interp->get_int("Key")) == 0)
        return true;
    
    if (key == key_ctrl_h || key == key_backspace) {
        if (max_pos == 0 || cursor_pos == 0)
            ; //status->setf ("Nothing to delete");
        else if (cursor_pos == max_pos) {
            max_pos--;
            cursor_pos--;
        }
        else { // we are in the middle of the input line
            memmove(input_buf + cursor_pos - 1, input_buf + cursor_pos, max_pos - cursor_pos);
            cursor_pos--;
            max_pos--;
        }
        
        left_pos = max(0,left_pos-1);
        
    }
    else if (key == key_ctrl_a) {
        cursor_pos = left_pos = 0;
    }
    else if (key == key_ctrl_c) { // save line to history but don't execute
        if (strlen(input_buf) > 0) {
            history->add (id, input_buf);
            set("");
            status->setf("Line added to history but not sent");
        }
    }
    else if (key == key_ctrl_j || key == key_ctrl_k) { // delete until EOL
        max_pos = cursor_pos;
    }
    else if (key == key_escape) {
        set("");
    }
    else if (key == key_ctrl_e) { // go to eol
        cursor_pos = max_pos;
        adjust();
    }
    else if (key == key_ctrl_u) {
        memmove(input_buf, input_buf+cursor_pos, max_pos - cursor_pos);
        max_pos -= cursor_pos;
        cursor_pos = 0;
        adjust();
    }
    else if (key == key_ctrl_w) { // Delete word
        // How long is the word?
        int bow = cursor_pos - 1;
        
        while (bow > 0 && isspace(input_buf[bow]))
            bow--;
        while (bow > 0 && !isspace(input_buf[bow]))
            bow--;

        if (bow > 0)
            bow++; // Don't eat the space

        if (bow >= 0 ) {
            memmove(input_buf+bow, input_buf+cursor_pos, max_pos - cursor_pos);
            max_pos -= cursor_pos - bow;
            cursor_pos = bow;
            adjust();
        }
    }
    else if (key == key_delete) { // delete to the right
        if (cursor_pos == max_pos)
            status->setf ("Nothing to the right of here");
        else  {
            memmove(input_buf+cursor_pos, input_buf + cursor_pos + 1, max_pos - cursor_pos);
            max_pos--;
        }
    }
    else if (key == keyEnter || key == key_kp_enter) { // return. finish this line
        ready = true;
        input_buf[max_pos] = NUL;
        
        if ((int)strlen(input_buf) >= config->getOption(opt_histwordsize))
            history->add (id, input_buf);
        
        history_pos = 0; // Reset history cycling
        cursor_pos = 0;
        
        max_pos = left_pos = 0;
        ready = false;
        
        move(0, parent->height-1);
        outputWindow->move(0,0);
        resize(width, 1);
        
        execute();
    }
    
    else if (key >= ' ' && key < 256) { // Normal key. Just insert
        if (max_pos < MAX_INPUT_BUF-1) {
            if (cursor_pos == max_pos) { // We are already at EOL
                input_buf[max_pos++] = key;
                cursor_pos++;
            } else { // We are inserting somewhere in the middle
                memmove(input_buf + cursor_pos +1, input_buf + cursor_pos, max_pos - cursor_pos);
                max_pos++;
                input_buf[cursor_pos++] = key;
            }
            adjust();
            
        }
        else
            status->setf ("The input buffer is full");
    }
    else if (key == key_arrow_left) {
        if (cursor_pos == 0)
            status->setf ("Already at the far left of the input line.");
        else
        {
            cursor_pos--;
            left_pos = max(0,left_pos-1);
        }
    }
    else if (key == key_arrow_right) {
        if (cursor_pos == max_pos)
            status->setf ("Already at the end of the input line.");
        else
        {
            cursor_pos++;
            if (cursor_pos > 7*width/8) // scroll only when we are approaching right margin
                adjust();
        }
    }
    else if (key == key_arrow_up) { // recall
        int lines=0;
        
        if (id == hi_none)
            status->setf ("No history available");
        else if (id != hi_generic && (lines = config->getOption(opt_historywindow)))
        {
            lines = min (parent->height-4, lines);
            
            if (!history->get(id,1, NULL))
                status->setf ("There are no previous commands");
            else
            {
                if (lines > 3)
                    (void)new InputHistorySelection(parent, width, lines, 0, -(lines+2), *this, true, id);
                // Window is to small; we need to cycle history in the input box
                else  {
                    (void)new MessageBox(screen, "Sorry, no history available", 0);
                }
            }
        }
        else {
            const char *s;
            if (!(s = history->get(id, history_pos+1, NULL)))
                status->setf ("No previous history");
            else  {
                set(s);
                history_pos++;
            }
        }
    }
    else if (key == key_arrow_down) {
        const char *s;
        if (id == hi_none)
            status->setf("No history available");
        else if (history_pos <= 1 || !(s = history->get(id,history_pos-1, NULL)))
        {
            status->setf ("No next history");
            history_pos = 0;
            set("");
        }
        else
        {
            set(s);
            history_pos--;
        }
    }
    
    else
        return false;
    
    input_buf[max_pos] = NUL;
    
    return true;
}

void InputLine::redraw() {
    int prompt_len = strlen(prompt_buf);
    
    gotoxy(0,0);
    set_color(config->getOption(opt_inputcolor));
    input_buf[max_pos] = NUL;

    if (config->getOption(opt_multiinput) && isExpandable()) {
        printf("%s%s%*s", prompt_buf, input_buf, (height*width)-prompt_len-max_pos, "");
        if (is_focused())
            set_cursor((cursor_pos+prompt_len)%width, (cursor_pos+prompt_len)/width);
    } else {
        printf("%s%s%-*.*s", prompt_buf, left_pos ? "<" : "",
               width-1-prompt_len + (left_pos ? 0 : 1), 
               width-1-prompt_len + (left_pos ? 0 : 1), 
               input_buf+left_pos);
        
        if (is_focused())
            set_cursor(cursor_pos+prompt_len-left_pos + (left_pos ? 1 : 0) ,0);
    }
    
    
    dirty = false;
}

// If an input line is ready, move it over to buf
bool InputLine::getline(char *buf, bool fForce)
{
	if (!ready && !fForce)
		return false;
	else
	{
		ready = false;

		if (fForce)
			input_buf[max_pos]  = NUL;
			
		strcpy (buf, input_buf);
		max_pos = left_pos = 0;
		return true;
	}
}

void InputLine::adjust() {
    if (config->getOption(opt_multiinput) && isExpandable()) {
        while ( ((int)strlen(prompt_buf) + max_pos)/width >= height) {
            move(parent_x, parent_y - 1);
            resize(width, height+1);
            outputWindow->move(0, 1-height);
        }
    }
    else
        while (1 + (int) strlen (prompt_buf) + cursor_pos - left_pos >= width)
            left_pos++;
}

void  InputLine::set_prompt (const char *s) {
    const char *in;
    char   *out;
    
    for (in = s, out = prompt_buf; *in && out-prompt_buf < MAX_PROMPT_BUF-1; in++)
        if (*in == (signed char) SET_COLOR)
            in++;
        else if (*in == '\n' || *in == '\r')
            *out++ = ' ';
        else
            *out++ = *in;
    
    *out++ = NUL;
    
    
    dirty = true;
}

MainInputLine::MainInputLine()
:InputLine(screen, wh_full, 1, None, 0, -1, hi_main_input) {
    parent->focus(this);
}

void MainInputLine::execute() {
    hook.run(USERINPUT, input_buf);
    interpreter.add(input_buf);
    if (config->getOption (opt_echoinput))		// echo input if wanted
        outputWindow->printf ("%c>> %s\n", SOFT_CR, input_buf);
}
