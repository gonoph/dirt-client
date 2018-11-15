#ifndef DIRT_STATUS_H
#define DIRT_STATUS_H

#include "Window.h"

class StatusLine : public Window {
    char *message;			// Current message
    time_t end_time;		// When should this message be turned off?

 public:	
    bool sticky_status;	
    StatusLine(Window *parent);
    virtual void idle();				// Check if it is time to remove the text
    void set(const char *s);			// Set status lien to this message
    void setf(const char *fmt ...); 	// As above ,but formatted string

    virtual void redraw();
    NAME(StatusLine);
};

extern StatusLine *status;			// This is the status line

#endif
