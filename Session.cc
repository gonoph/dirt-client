// Sesssion.cc
// This defines a Session structure which communicates with a MUD

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>     // for INT_MIN
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/telnet.h>
#include "dirt.h"
#include "cui.h"
#include "Session.h"
#include "Interpreter.h"
#include "Action.h"
#include "Alias.h"
#include "Curses.h"
#include "Chat.h"
#include "Hook.h"

const int connectTimeout = 30;

// Network window
class NetworkStateWindow : public Window {
	public:
	
	NetworkStateWindow (Session& _ses)
	: Window(screen, 19,1, None, -18, 1),
	ses(_ses) {}

	private:
	virtual void redraw();
	virtual bool keypress(int key);
	Session& ses;
};

enum show_t {show_clock, show_clock_sec, show_timer, show_timer_sec, max_show_t };

int timer_show [8][max_show_t] = {
	{1,1,1,1},
	{1,0,1,1},
	{1,0,1,0},
	{1,1,0,0},
	{1,0,0,0},
	{0,0,1,1},
	{0,0,1,0},
	{-1,-1,-1,-1}
};

// Convert to X, xK, xM, xG
const char* csBytes(int n) {
    int letter = ' ';
    double f  = n;
    if (f > 1024) {
        f /= 1024;
        letter = 'k';
    }
    if (f > 1024) {
        f /= 1024;
        letter = 'm';
    }

    if (letter == ' ' )
        return Sprintf("%.0f", f);
    else
        return Sprintf("%.1f%c", f, letter);
}

class StatWindow : public Window {
public:
    StatWindow(Session &_ses) : Window(screen, 15, 1, None, -15, 3),
        ses(_ses), last_bytes_written(-1), last_bytes_read(-1) {
        }

    virtual void idle() {
        if (last_bytes_written != ses.stats.bytes_written
            || last_bytes_read != ses.stats.bytes_read) {
            force_update();
        }
    }

    virtual void redraw() {
        set_color(config->getOption(opt_statcolor));
        clear();
        gotoxy(0,0);
        printf("%7s/%7s", csBytes(ses.stats.bytes_read), csBytes(ses.stats.bytes_written));
        last_bytes_written = ses.stats.bytes_written;
        last_bytes_read = ses.stats.bytes_read;
        dirty = false;
    }
    
private:
    Session& ses;
    int last_bytes_written, last_bytes_read;
};

class TimerWindow : public Window {
	public:
	TimerWindow(Session &_ses) :Window (screen, 17, 1, None, -17, 2 ),
		ses(_ses), state(config->getOption(opt_timerstate)) {}
	
	private:
	Session& ses;
	virtual void redraw();
	virtual void idle();
	virtual bool keypress(int key);
	time_t last_update;
	
	int state, last_state;
};

void TimerWindow::idle() {
	if (last_update != current_time)
		force_update();
}

void TimerWindow::redraw () {
	last_state = state;
	gotoxy (0, 0);
	set_color (config->getOption (opt_timercolor));

	// Adjust dimensions based on what we have to show
	height = 1;
	width = 0;

	if (timer_show[state][show_clock])
		width += 5;

	if (timer_show[state][show_clock_sec])
		width += 3;

	if (timer_show[state][show_timer])
		width += 6;

	if (timer_show[state][show_timer_sec])
		width += 3;

	if (timer_show[state][show_clock] && timer_show[state][show_timer])
        width++;

    resize(width, height);
    move(parent->width - width, parent_y);

	clear ();
	last_update = current_time;

	if (timer_show[state][show_clock]) 	{
		struct tm *tm = localtime (&current_time);

		printf ("%02d:%02d", tm->tm_hour, tm->tm_min);

		if (timer_show[state][show_clock_sec])
			printf (":%02d", tm->tm_sec);
	}

	if (timer_show[state][show_clock] && timer_show[state][show_timer])
		print (" ");

	if (timer_show[state][show_timer]) {
		int     difference = int (difftime (current_time, ses.stats.dial_time));

		printf ("%03d:", difference / (60 * 60));
		difference -= (difference / (60 * 60)) * 60 * 60;
		printf ("%02d", difference / 60);

		if (timer_show[state][show_timer_sec])
			printf (":%02d", difference % 60);
	}

	dirty = false;
}

bool TimerWindow::keypress(int key) {
	if (key == key_ctrl_t) {
		dirty = true;

		// Die if we reach the end of the table
		if (timer_show[++state][show_clock] < 0) {
			ses.timer = NULL;
			die();
		}

		return true;
	}
	else
		return false;
}


void  NetworkStateWindow::redraw () {
    int     tx_queue, rx_queue;
    int     timer, retrans;
    
    set_color(config->getOption(opt_statcolor));
    clear ();
    gotoxy(0,0);

    switch (mudcompress_version(ses.mcinfo)) {
    case 0:
        printf("  ");
        break;
    case 1:
        printf("c ");
        break;
    case 2:
        printf("C ");
        break;
    default:
        printf("C?");
        break;
    }
    
    if (ses.state == disconnected)
        printf ("Offline"); // Hmm, this cannot really happen
    else
        if (ses.get_connection_stats (tx_queue, rx_queue, timer, retrans))
            printf ("%4d %2d %5.1f/%2d",
                    tx_queue, rx_queue, timer/100.0, retrans);
    
    dirty = false;
}

bool  NetworkStateWindow::keypress(int key) {
	if (key == key_alt_s) {
        ses.statWindow->die();
        ses.statWindow = NULL;

        ses.nsw = NULL;
        die();
        
		return true;
	}
	else
		return false;
}

Session::Session(MUD& _mud, Window *_window, int _fd) : Socket(_fd), state(disconnected),mud(_mud), window(_window),
nsw(NULL), timer(NULL),  statWindow(NULL), last_nsw_update(0)
{
    input_buffer[0] = NUL;
    prompt[0] = NUL;
    memset(&stats,0,sizeof(stats));

    if (config->getOption(opt_autostatwin))
            show_nsw();
    
    if (config->getOption(opt_autotimerwin))
            show_timer();

    mcinfo = mudcompress_new();
    if (!mud.loaded) {
        mud.loaded = true;
        embed_interp->load_file(mud.name, true);
    }

    embed_interp->set("mud", mud.name);
    hook.add(SEND, new CppHookStub(INT_MIN, 1.0, -1, false, true, true, "__DIRT_BUILTIN_writeMUD", 
        vector<string>(1,"Dirt builtins"), &writeMUD));
    hook.run(CONNECT);
//    embed_interp->eval("hook_run('connect')", "", NULL);

    if (_fd != -1) {
        stats.dial_time = current_time;
        establishConnection(true);
    }
}

Session::~Session() {
    close();
    
    if (nsw)
        nsw->die();
    
    if (timer)
        timer->die();

    if (statWindow)
        statWindow->die();
    
    unsigned long comp, uncomp;
    mudcompress_stats(mcinfo, &comp, &uncomp);
    globalStats.comp_read += comp;
    globalStats.uncomp_read += uncomp;
    
    mudcompress_delete(mcinfo);

    set_title("dirt - unconnected");
}


// Write to whatever we are connected to
// Also log, if logging is active?
void Session::print (const char *s) {
	if (window)
		window->print(s);
}

// Try to connect to mud
bool Session::open() {
    int res = connect(mud.getHostname(), mud.getPort(), true);

    if (res != errNoError && res != EINPROGRESS) {
		status->setf ("%s - error ocurred: %s", mud.getFullName(), getErrorText());
		return false;
	}

	status->setf ("Connecting to %s", mud.getFullName());
	state = connecting;
    stats.dial_time = current_time;

    set_title(Sprintf("dirt - connecting to %s", mud.getFullName()));
	return true;
}

// Disconnect from mud
bool Session::close() {
	if (state > disconnected) { // Closing a closed session has no effect
            state = disconnected;
            hook.run(LOSELINK);
            hook.remove("__DIRT_BUILTIN_writeMUD");
	}

    embed_interp->set("mud", "");
	return true;
}

bool writeMUD(string& s) {
    s = debackslashify(s);
    currentSession->writeLine(s.c_str());
    currentSession->stats.bytes_written += s.length();
    globalStats.bytes_written += s.length();
    return true;
}

bool writeMUDn(string& s) {  // no CR added...
    s = debackslashify(s);
    currentSession->write(s.c_str(), s.length());
    currentSession->stats.bytes_written += s.length();
    globalStats.bytes_written += s.length();
    return true;
}

// Do various time updates
void Session::idle() {
    if (state == connecting) {
        int time_left = stats.dial_time - current_time + connectTimeout;
        if (time_left <= 0) {
            close();
            status->setf ("Connection to %s timed out", mud.getFullName());
        }  else  {
            static char filled_string[64];
            static char empty_string[64];
            char buf[256];
            
            if (!filled_string[0]) {
                for (unsigned int i = 0; i < sizeof(filled_string)-1; i++) {
                    filled_string[i] = special_chars[sc_filled_box];
                    empty_string[i] = special_chars[sc_half_filled_box];
                }
            }
            
            sprintf(buf,"Connecting to %s %-.*s%-.*s", mud.getFullName(),
                    connectTimeout-time_left+1, filled_string,
                    time_left-1,         empty_string);
            status->setf (buf);
        }
    }
    
    if (nsw && last_nsw_update != current_time) {
        nsw->force_update();
        last_nsw_update = current_time;
    }
}

void Session::set_prompt (const char *s, int len) {
    char buf[MAX_MUD_BUF];
    memcpy(buf, s, len);
    buf[len] = NUL;
//    embed_interp->eval("hook_run('prompt')", buf, buf);
    hook.run(PROMPT, buf);
    inputLine->set_prompt(buf);
}

void Session::establishConnection (bool quick_restore) {
    state = connected;
    stats.connect_time = current_time;
    
    // Send commands, if any
    if (!quick_restore && mud.commands.len())
        interpreter.add(mud.commands);

    char buf[256];
    sprintf(buf, "dirt - %s", mud.getFullName());
    set_title(buf);
}

void Session::connectionEstablished() { // Called from Socket
    establishConnection(false);
    status->setf("Connection to %s successful", mud.getFullName());
}

void Session::errorEncountered(int) {
    status->setf ("%s - %s", mud.getFullName(), getErrorText());
    close();
}

// Data from the MUD has arrived
void Session::inputReady() {
    char    temp_buf[MAX_MUD_BUF];
    unsigned char   *lastline = (unsigned char*)input_buffer;
    // These are used in processing the input.  In case of a partial line, they
    // are static so they can be used when the rest of the line is received.
    static int     pos=0;
    static char    out_buf[MAX_MUD_BUF];
    static char   *out;                   // points into out_buf
    static char   *prompt_begin = NULL;   // Points WRT out
    static int     code_pos = -1;         // indicates where inside an ANSI sequence we are.
                                   // -1 indicates we're not inside an ansi
                                   // sequence.
    
    int     count;
    int     i;

    count = read(temp_buf, MAX_MUD_BUF-1);

    globalStats.bytes_read += count;
    stats.bytes_read += count;
    
    // Filter through mudcompress
    mudcompress_receive(mcinfo, temp_buf, count);
    
    // Error?
    if (mudcompress_error(mcinfo)) {
        close();
        status->setf ("%s - compression error", mud.getFullName());
        return;
    }
    
    // Need to respond?
    const char *mc_response = mudcompress_response(mcinfo);
    while (mc_response) {
        write(mc_response, strlen(mc_response));
        mc_response = mudcompress_response(mcinfo);
    }
    
    while (mudcompress_pending(mcinfo) && pos < MAX_MUD_BUF-1) {
        // Get some data
        count = mudcompress_get(mcinfo, (char*) input_buffer + pos, MAX_MUD_BUF - pos - 1);

        if (count > 0 && chatServerSocket)
            chatServerSocket->handleSnooping((char*)(input_buffer+pos), count);

        /* Process the buffer */
        for (i = pos; i < count + pos; i++)
        {
            /* Lose patience of code does not terminate within 16 characters */
            if (code_pos >= 0 && i - code_pos > 16)
                code_pos = -1;
            
            
            /* IAC: next character is a telnet command */
            if (input_buffer[i] == IAC) {
                report("Found telnet IAC.\n");
                if (++i < count + pos) {	/* just forget it if it appears at the end of a buffer */
                    /* spec: handle prompts that split across reads */
                    if (input_buffer[i] == GA || input_buffer[i] == EOR) { /* this is a prompt */
                        /* if we have a prompt_begin, that's the start of
                         * the prompt. If we don't, then the contents
                         * of the 'prompt' buffer, plus any output we
                         * have, is the prompt.
                         */
                        if(!config->getOption(opt_snarf_prompt)) {
                            if(prompt_begin) {
                                int len = out - prompt_begin;
                                char *buf = new char[len + 1];
                                memcpy(buf, prompt_begin+1, len);
                                buf[len] = '\0';
                                hook.run(PROMPT, buf);
                            }
                        } else if (prompt_begin) {
                            set_prompt (prompt_begin + 1, out - prompt_begin - 1);
                            if (!config->getOption(opt_showprompt))
                                out = prompt_begin + 1;
                        } else {
                            if (prompt[0] || out[0]) {
                                unsigned char   *temp = prompt + strlen ((char*)prompt);
                                
                                *out = NUL;
                                strcat ((char*)prompt, out_buf);
                                set_prompt ((char*)prompt, (int)strlen ((char*)prompt));
                                *temp = NUL;
                            }
                            if (!config->getOption(opt_showprompt))
                                out = out_buf;
                        }
                        // Insert a clear color code here
                        // It'd be better to interpret color codes in the prompt properly,
                        // but that is surprisingly hard to do FIXME Huh?
                        *out++ = SET_COLOR;
                        *out++ = bg_black|fg_white; // Is that really the *default* color?
                        
                        prompt[0] = NUL;            // Make prompt empty.
                        prompt_begin = out;         // Mark current position as start of prompt
                    }
                    // React to IAC WILL EOR and send back IAC DO EOR
                    else if (input_buffer[i] == WILL && (i+1) < count+pos && input_buffer[i+1] == TELOPT_EOR) {
                        i++;
                        
                        // FIXME use telnet.h defines here
                        write ("\377\375\31", 3);
                    }
                    /* Skip the next character if this is an option */
                    else if (input_buffer[i] >= WILL && input_buffer[i] <= DONT)
                        i++;
                }
                
                continue;
            }
            
            // Escape sequence
            else if (input_buffer[i] == '\e')		
                code_pos = i;
            
            // Attention
            else if (input_buffer[i] == '\a' && config->getOption(opt_mudbeep))
                ::write(STDOUT_FILENO, "\a", 1); // use screen->flash() here?
            
            else if (code_pos == -1) { // not inside a color code, real text
                if (input_buffer[i] == '\n')  {
                    int len = prompt_begin ? out-prompt_begin-1: out-out_buf;
                    int old_len = len;
                    char* line = prompt_begin ? prompt_begin +1 : (char*) out_buf;
                    
                    line[len] = NUL;                 // This should replace a \n with a \0
                    hook.run(OUTPUT, line);
                    out = line + strlen(line);
                    len = strlen(line);
                    interpreter.execute(); // If triggers generated any new commands, execute them.

                    prompt_begin = out;
                    *out='\0';      // Make sure it's null terminated
                    
                    // if we had a >0 char line and now have 0 line.. the line
                    // shouldn't be shown at all.
                    if (old_len > 0 && len == 0) {
                        if (out > out_buf)
                            prompt_begin = out-1;
                        else
                            prompt_begin = NULL;
                        continue;
                    } else {
                        out[0] = '\n';
                        out[1] = '\0';
                        print(out_buf); // FIXME print one line.
                        prompt_begin=NULL;
                    }
                    out = out_buf;  // reset pointer -- use this buffer over.
                    lastline = input_buffer + i+1;
                } else if (input_buffer[i] != '\r')	/* discard those */
                    *out++ = input_buffer[i];	/* Add to output buffer */
            }
            
            /* Check if the code should terminate here */
            if (code_pos >= 0 && isalpha (input_buffer[i])) {
                int     color;
                
                /* Conver this color code to internal representation */
                if ((color = colorConverter.convert (input_buffer + code_pos, i - code_pos + 1)) > 0) {
                    *out++ = SET_COLOR;
                    *out++ = color;
                }
                if (colorConverter.checkReportStatus()) { /* suggested by Chris Litchfield */
                    outputWindow->printf("\n(Sending location code\n");
                    hook.run(SEND, "\e[40;13R\n");
//                    writeMUD("\e[40;13R\n");
                }
                
                code_pos = -1;
            }
        } // for(... each byte in buffer ...)
        
        *out = NUL;

        /* Do we have some leftover data, an incomplete ANSI sequence? */
        if(lastline != input_buffer + i) {
            pos = count+pos-(lastline-(unsigned char*)input_buffer);
            memcpy(input_buffer, lastline, pos);
            input_buffer[pos] = '\0';
        } else {
            pos = 0;
            if(code_pos != -1) {
                report_err("We have code_pos but buffer terminates on line-end!\n");
            }
        }

        /* spec: fix up partial lines for subsequent prompts */
        if (prompt_begin)
            strcpy ((char*)prompt, prompt_begin);
        else if (strlen((char*)prompt) < MAX_MUD_BUF/4 && strlen(out_buf) < MAX_MUD_BUF/4) {
            // guard against too long lines
            strcat((char*)prompt, out_buf);
        }
        input_buffer[pos] = '\0';


    } // end while
}

void Session::show_timer() {
	timer = new TimerWindow(*this);
}

void Session::show_nsw() {
    nsw = new NetworkStateWindow(*this);
    statWindow = new StatWindow(*this);
}

bool Session::expand_macros(int key) {
    if (macros_disabled)
        return false;
    
    // This is a bit primitive currently
    Macro *m = mud.findMacro(key);
    if (m) {
        if (config->getOption(opt_echoinput))  {
            char buf[256];
            
            snprintf (buf, sizeof(buf), "%c>> %s -> %s\n",
                      SOFT_CR, key_name(m->key), ~m->text);
            print(buf);
        }
        
        interpreter.add(m->text);
        return true;
    }
    
    return false;
}

