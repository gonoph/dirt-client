#ifndef __SESSION_H
#define __SESSION_H

#include "mccpDecompress.h"
#include "Socket.h"

typedef enum {disconnected, connecting, connected} state_t;

class NetworkStateWindow;
class TimerWindow;
class StatWindow;
class Window;

class Session : public Socket {
friend bool writeMUD(string& s);
friend bool writeMUDn(string& s);
  public:
    Session(MUD& _mud, Window *_window, int _fd = -1);	// Initialize a new session to this mud
    ~Session();
    bool open();							// Connect
    bool close();							// Disconnect
    void idle();							// Do time-based updates
    
    void show_nsw();						// Show the NSW
    void show_timer();						// Show the Timer Window
    bool expand_macros(int key);			// Expand this macro key
//    bool writeMUD (const char *s);	     	// Send this
    
    state_t state;			// Are offline, connecting or connected?
    MUD& mud;
    
private:
    Window *window;			// Window we are writing to
    
    // Incomplete ANSI codes should be left in this buffer 
    unsigned char input_buffer[MAX_MUD_BUF];
    
    // and this one, partial lines (for prompts) 
    unsigned char prompt[MAX_MUD_BUF];
    int pos;
    
    NetworkStateWindow *nsw;	// if non-NULL, we display stats there when idle'ing
    TimerWindow *timer;			// Timer window
    StatWindow *statWindow;     // Input/Output statitstics here
	
    // Some statistics
    struct {
	int bytes_written, bytes_read;	// # of bytes read/written
	time_t connect_time;			// When did we connect?
	time_t dial_time;
    } stats;
    
    time_t last_nsw_update;
    mc_state *mcinfo;
    ColorConverter colorConverter;

    friend NetworkStateWindow;
    friend StatWindow;
    friend TimerWindow;

    void print(const char *s);				// Write to our output/log

    void set_prompt (const char *s, int len);
    bool triggerCheck (char *line, int len, char **out);
    void establishConnection(bool quick_restore); // run on connect, qr = true if 'hot' boot
    int  convert_color (const unsigned char *s, int size);

    virtual void connectionEstablished();
    virtual void inputReady();
    virtual void errorEncountered(int);

};

extern Session *currentSession;

#endif
