// The input line
#include "InputLine.h"
#include "misc.h"
#include "TTY.h"
#include "Interpreter.h"
#include "EmbeddedInterpreter.h"
#include "Screen.h"
#include "Config.h"
#include "Color.h"
#include "Selection.h"
#include "OutputWindow.h"
#include "StatusLine.h"
#include "StaticBuffer.h"
#include "MessageBox.h"

#include <sys/stat.h>
#include <fstream>
#include <iostream>

History::History(int _id) : id (_id), current(0) {
    max_history = config->getOption(opt_histsize);
    strings = new string[max_history];
    timestamps = new time_t[max_history];
}

History::~History() {
    delete[] strings;
    delete[] timestamps;
}

void History::add(const string& s,time_t t) {
    // Don't store multiple strings that are exactly the same
    if (current > 0 && s == strings[(current-1) % max_history])
            return;

    strings[current % max_history] = s;
    timestamps[current % max_history] = t;

	current++;
}

// getting number 1 gets you the LAST line
const char * History::get(int count, time_t* timestamp) {
    if (count > min(current, max_history))
        return NULL;
    
    if (timestamp)
        *timestamp = timestamps[(current - count) % max_history];
    return strings[(current - count) % max_history].c_str();
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
             char buf[4096];

             while (3 == fscanf(fp, "%d %ld %1024[^\n]", &id, &t, buf))
                hist_list[id]->add(buf,t);
             fclose(fp);
         }
/*
        ifstream hin(Sprintf("%s/.dirt/history", getenv("HOME")));
        if (hin.is_open()) {
            int id;
            time_t t;
            char buf[4096], c;

            //while(in.peek() != in.eof()) {
            while(!hin.eof()) {
                hin.getline(buf, 4096);  // Dies right here...never returns...
//                cout << "Got history line: " << buf << endl;
                istringstream sbuf(buf);
                sbuf >> id >> t;
                sbuf.get(c); // skip the leading space.
                sbuf.getline(buf,4096);
                hist_list[id]->add(buf, t);
            }
            hin.close();
        }
*/
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
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_a", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_a, "", &InputLine::keypress_ctrl_a, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_c", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_c, "", &InputLine::keypress_ctrl_c, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_k", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_k, "", &InputLine::keypress_ctrl_k, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_j", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_j, "", &InputLine::keypress_ctrl_k, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_escape", vector<string>(1, "Dirt keys"), 
        "", "", key_escape, "", &InputLine::keypress_escape, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_backspace", vector<string>(1, "Dirt keys"), 
        "", "", key_backspace, "", &InputLine::keypress_backspace, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_h", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_h, "", &InputLine::keypress_backspace, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_e", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_e, "", &InputLine::keypress_ctrl_e, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_u", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_u, "", &InputLine::keypress_ctrl_u, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_ctrl_w", vector<string>(1, "Dirt keys"), 
        "", "", key_ctrl_w, "", &InputLine::keypress_ctrl_w, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_delete", vector<string>(1, "Dirt keys"), 
        "", "", key_delete, "", &InputLine::keypress_delete, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_enter", vector<string>(1, "Dirt keys"), 
        "", "", key_enter, "", &InputLine::keypress_enter, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_kp_enter", vector<string>(1, "Dirt keys"), 
        "", "", key_kp_enter, "", &InputLine::keypress_enter, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_arrow_left", vector<string>(1, "Dirt keys"), 
        "", "", key_arrow_left, "", &InputLine::keypress_arrow_left, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_arrow_right", vector<string>(1, "Dirt keys"), 
        "", "", key_arrow_right, "", &InputLine::keypress_arrow_right, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_arrow_up", vector<string>(1, "Dirt keys"), 
        "", "", key_arrow_up, "", &InputLine::keypress_arrow_up, (void*)this));
    hook.add(KEYPRESS, new KeypressHookStub(-1, 1.0, -1, false, true, true, 
        "__DIRT_BUILTIN_InputLine::keypress_arrow_down", vector<string>(1, "Dirt keys"), 
        "", "", key_arrow_down, "", &InputLine::keypress_arrow_down, (void*)this));
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
    
//    report("calling hook.run(KEYPRESS, %s)", input_buf);
    hook.run(KEYPRESS, input_buf); // FIXME MOVE to TTY::check_fdset
//    report("returned from hook.run(KEYPRESS, %s)", input_buf);
    max_pos = strlen(input_buf);
    cursor_pos = min(max_pos, max(cursor_pos, 0));
    adjust();
    
    // set Key to 0 if keypress handled. Ugh FIXME: make all of the below hooks.
    if ((key = embed_interp->get_int("Key")) == 0)
        return true;
    
    if (key >= ' ' && key < key_backspace) { // Normal key. Just insert
//    if (key >= ' ' && key < 127) { // Normal key. Just insert
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
        input_buf[max_pos] = NUL;
        return true;
    }
    return false;
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
        int newheight = 1 + (strlen(prompt_buf) + max_pos)/width;
        int adjust = newheight - height;
        status->setf("height: %d, adjust = %d, 1-(height+adjust)=%d", height, adjust, 1-(height+adjust));
        if(adjust) {
        // FIXME: This doesn't work.
        // There is some strange interactions going on at the Window level when I resize things.
        // For instance, after the outputWindow->move() call, the HEIGHT of the InputLine is changed.
            move(parent_x, parent_y - adjust);
            report("move(%d, %d)", parent_x, parent_y-adjust);
            outputWindow->move(0, 1-(height+adjust));
            report("outputWindow->move(0, %d)", 1-(height+adjust));
            resize(width, height+adjust);
            report("resize(%d, %d) height: %d, adjust: %d", width, height+adjust, height, adjust);
            //outputWindow->resize(outputWindow->width, outputWindow->height - adjust); // causes segfault.
            //parent->resize(parent->width, parent->height - adjust);
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

// go to beginning of line.
bool InputLine::keypress_ctrl_a(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    mythis->cursor_pos = mythis->left_pos = 0;
    return true;
}

// save line to history but don't execute
bool InputLine::keypress_ctrl_c(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    if(inputline.length()) {
        history->add (mythis->id, mythis->input_buf);
        inputline = "";
        status->setf("Line added to history but not sent");
    }
    return true;
}

// delete until EOL
bool InputLine::keypress_ctrl_k(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    inputline.erase(mythis->cursor_pos);
    return true;
}

// erase input line
bool InputLine::keypress_escape(string& inputline, void*) {
    inputline = "";
    return true;
}

// delete one character to left
bool InputLine::keypress_backspace(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    if (mythis->max_pos != 0 && mythis->cursor_pos != 0) {
        inputline.erase(--mythis->cursor_pos, 1);
        mythis->left_pos = max(0,mythis->left_pos-1);
    }
    return true;
}

// go to end of line.
bool InputLine::keypress_ctrl_e(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    mythis->cursor_pos = mythis->max_pos;
    return true;
}

// Delete one word
bool InputLine::keypress_ctrl_w(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    // How long is the word?
    int bow = mythis->cursor_pos - 1;
    
    while (bow > 0 && isspace(inputline[bow])) bow--;   // Scan to end of word (skip space)
    while (bow > 0 && !isspace(inputline[bow])) bow--;  // Scan to beginning of word
    if (bow > 0) bow++; // Don't eat the space
    if (bow >= 0 ) {
        inputline.erase(bow, mythis->cursor_pos - bow);
        mythis->cursor_pos = bow;
    }
    return true;
}

// delete to beginning of line
bool InputLine::keypress_ctrl_u(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    inputline.erase(0, mythis->cursor_pos);
    mythis->cursor_pos = 0;
    return true;
}

// delete one char to the right
bool InputLine::keypress_delete(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    if (mythis->cursor_pos != mythis->max_pos)
        inputline = inputline.substr(0,mythis->cursor_pos) 
            + inputline.substr(mythis->cursor_pos+1, mythis->max_pos);
    return true;
}

// execute
bool InputLine::keypress_enter(string& inputline, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    mythis->ready = true;

    if(inputline.length() >= (unsigned)config->getOption(opt_histwordsize)) {
        history->add (mythis->id, mythis->input_buf);
    }
    
    mythis->history_pos = 0; // Reset history cycling
    mythis->cursor_pos = 0;
    mythis->left_pos = 0;
    mythis->ready = false;
    hook.run(USERINPUT, inputline);
    if(inputline[0] == interpreter.getCommandCharacter())
        hook.run(COMMAND, inputline);
    else hook.run(SEND, inputline);
    // FIXME interpreter.add(inputline);
    if (config->getOption (opt_echoinput))		// echo input if wanted
        outputWindow->printf ("%c>> %s\n", SOFT_CR, inputline.c_str());
    inputline = "";
    return true;
}

bool InputLine::keypress_arrow_left(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    if (mythis->cursor_pos == 0)
        status->setf ("Already at the far left of the input line.");
    else
    {
        mythis->cursor_pos--;
        mythis->left_pos = max(0,mythis->left_pos-1);
    }
    return true;
}

bool InputLine::keypress_arrow_right(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    if (mythis->cursor_pos == mythis->max_pos)
        status->setf ("Already at the end of the input line.");
    else
    {
        mythis->cursor_pos++;
        if (mythis->cursor_pos > 7*mythis->width/8) // scroll only when we are approaching right margin
            mythis->adjust();
    }
    return true;
}

bool InputLine::keypress_arrow_up(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    int lines=0;
    
    if (mythis->id == hi_none)
        status->setf ("No history available");
    else if (mythis->id != hi_generic && (lines = config->getOption(opt_historywindow)))
    {
        lines = min (mythis->parent->height-4, lines);
        
        if (!history->get(mythis->id,1, NULL))
            status->setf ("There are no previous commands");
        else
        {
            if (lines > 3)
                (void)new InputHistorySelection(mythis->parent, mythis->width, 
                                                lines, 0, -(lines+2), *mythis, true, mythis->id);
            // Window is to small; we need to cycle history in the input box
            else  {
                (void)new MessageBox(screen, "Sorry, no history available", 0);
            }
        }
    }
    else {
        const char *s;
        if (!(s = history->get(mythis->id, mythis->history_pos+1, NULL)))
            status->setf ("No previous history");
        else  {
            mythis->set(s);
            mythis->history_pos++;
        }
    }
    return true;
}

bool InputLine::keypress_arrow_down(string&, void* mt) {
    InputLine* mythis = (InputLine*)mt;
    const char *s;
    if (mythis->id == hi_none)
        status->setf("No history available");
    else if (mythis->history_pos <= 1 || !(s = history->get(mythis->id,mythis->history_pos-1, NULL)))
    {
        status->setf ("No next history");
        mythis->history_pos = 0;
        mythis->set("");
    }
    else
    {
        mythis->set(s);
        mythis->history_pos--;
    }
    return true;
}

MainInputLine::MainInputLine()
:InputLine(screen, wh_full, 1, None, 0, -1, hi_main_input) {
    parent->focus(this);
}
