// Implementation of a status line

#include "dirt.h"
#include "StatusLine.h"
#include <stdarg.h>

#define STATUSLINE_TIMEOUT 5


StatusLine::StatusLine(Window *_parent)
	: Window(_parent, wh_full,1),
		message(NULL), end_time(0) 
{
    sticky_status = false;
}

void StatusLine::idle()
{	
	if (message && current_time > end_time && !sticky_status)
	{
		free(message);
		message = NULL;
		show(false);
	}
}


// Set the status line (using a formatted string)
void StatusLine::setf(const char *fmt ...)
{
	char buf[256];
	va_list va;
	
	va_start(va,fmt);
	vsnprintf (buf, 256, fmt, va);
	va_end(va);	
	set(buf);
}

void StatusLine::set(const char *s)
{
    if (message)
        free(message);
    
    message = strdup(s);
    end_time = current_time + STATUSLINE_TIMEOUT;
    show(true);
}

void StatusLine::redraw()
{
	if (message)	
	{
		gotoxy(0,0);
		set_color (config->getOption(opt_statuscolor));
		printf("%-*.*s", width, width, message);
		dirty = false;
	}
}

