#include "dirt.h"
#include "misc.h"
#include "OutputWindow.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>


void error(const char *fmt, ...) {
    va_list va;

    freopen("/dev/tty", "r+", stderr);
    va_start(va,fmt);
    fputs(CLEAR_SCREEN,stderr);
	vfprintf(stderr, fmt,va);
	va_end(va);
	
    fprintf(stderr, "\n");
	
	exit (EXIT_FAILURE);
}

void report(const char *fmt, ...) {
    va_list va;
    int len = strlen(fmt);
    char buf[4096];
    char *newfmt = (char*)malloc(len+4);
    newfmt[0] = '@';
    newfmt[1] = ' ';
    strcpy(newfmt+2, fmt);
    if(newfmt[len+1] != '\n') {
        newfmt[len+2] = '\n';
        newfmt[len+3] = '\0';
    }
    va_start(va,fmt);	
    vsnprintf(buf, sizeof(buf)-1, newfmt,va);
    va_end(va);

    if (outputWindow) {
        outputWindow->print(buf);
    } else
        fprintf(stderr, "%s", buf);
    free(newfmt);
}

void report_warn(const char *fmt, ...) {
    va_list va;
    int len = strlen(fmt);
    char buf[4096];
    char *newfmt = (char*)malloc(len+25);
    strcpy(newfmt, "@ \xEA\xFF\xEA\x04[WARNING]\xEA\xFE\xEA\x07 ");
    len = strlen(newfmt);
    strcpy(newfmt+len, fmt);
    len = strlen(newfmt);
    if(newfmt[len-1] != '\n') {
        newfmt[len] = '\n';
        newfmt[len+1] = '\0';
    }
    va_start(va,fmt);	
    vsnprintf(buf, sizeof(buf)-1, newfmt,va);
    va_end(va);

    if (outputWindow) {
        outputWindow->print(buf);
    } else
        fprintf(stderr, "%s", buf);
    free(newfmt);
}

void report_err(const char *fmt, ...) {
    va_list va;
    int len = strlen(fmt);
    char buf[4096];
    char *newfmt = (char*)malloc(len+25);
    strcpy(newfmt, "@ \xEA\xFF\xEA\x04[ERROR]\xEA\xFE\xEA\x07 ");
    len = strlen(newfmt);
    strcpy(newfmt+len, fmt);
    len = strlen(newfmt);
    if(newfmt[len-1] != '\n') {
        newfmt[len] = '\n';
        newfmt[len+1] = '\0';
    }
    va_start(va,fmt);	
    vsnprintf(buf, sizeof(buf)-1, newfmt,va);
    va_end(va);

    if (outputWindow) {
        outputWindow->print(buf);
    } else
        fprintf(stderr, "%s", buf);
    free(newfmt);
}

string uncolorize(string& str) {
    string uncolored = str;
    for(size_t pos = uncolored.find(SET_COLOR); pos != string::npos; pos = uncolored.find(SET_COLOR, pos)) {
        uncolored.erase(pos, 2);
    }
    return uncolored;
}

string debackslashify(const string& s) {
    size_t lastpos=0,pos=0;
    string retval("");
    if((pos = s.find('\\', lastpos)) != string::npos) {
        do {
            retval += s.substr(lastpos, pos-lastpos);
            if(pos < s.length()-1) {
                retval += s.substr(pos+1, 1);
                lastpos = pos+2;
            } else {
                lastpos = pos+1; // what if this is last char?
            }
        } while((pos = s.find('\\', lastpos)) != string::npos);
        if(s.length()-lastpos > 0) {
            retval += s.substr(lastpos, s.length()-lastpos);
        }
        return retval;
    } else {
        return s;
    }
}

string backslashify(const string& s, char c) {
    size_t lastpos=0,pos=0;
//    string tmp("");
//    string retval(s);
    string tmp(s);
    string retval("");
    // First double each backslash ***only if it preceedes the character we're escaping!
//    if((pos = retval.find('\\', lastpos)) != string::npos) {
//        do {
//            tmp += retval.substr(lastpos, pos-lastpos);
//            tmp += "\\\\"; // TWO backslashes.
//            lastpos = pos+1;
//        } while((pos = retval.find('\\', lastpos)) != string::npos);
//        tmp += retval.substr(lastpos, retval.length()-lastpos);
//    } else tmp = s;
//    retval = "";
    lastpos = pos = 0;
    // Now escape c
    if((pos = tmp.find(c, lastpos)) != string::npos) {
        do {
            retval += tmp.substr(lastpos, pos-lastpos); // doesn't copy the c we found.
            retval += "\\";
            retval += c;
            lastpos = pos+1;
        } while((pos = tmp.find(c, lastpos)) != string::npos);
        retval += tmp.substr(lastpos, tmp.length()-lastpos);
    } else retval = tmp;
    return retval;
}

// #define MEMORY_DEBUG

#ifdef MEMORY_DEBUG

#define MAGIC_MARKER (int) 0xEAEAEAEA

#define FRONT(ptr) *(int*)ptr
#define SIZE(ptr) ((int*)ptr)[1]
#define BACK(ptr, size) *(int*)(ptr + size + 2 * sizeof(int))

void * operator new (size_t size)
{
    char *ptr = (char*) malloc(size+3*sizeof(int));
    FRONT(ptr) = MAGIC_MARKER;
    SIZE(ptr) = size;

    BACK(ptr,size) = MAGIC_MARKER;

//    fprintf(stderr, "Allocated   %6u bytes at %08x\n", size, (int)ptr);

    return ptr+(2*sizeof(int));
}

void operator delete (void *ptr)
{
    int size;
    char *p = ((char*)ptr)- (2 * sizeof(int));

    if (FRONT(p) != MAGIC_MARKER)
        abort();

    size = SIZE(p);

    if (BACK(p, size) != MAGIC_MARKER)
        abort();

    FRONT(p) = 0;
    

//    fprintf(stderr, "Deallocated %6d bytes at %08x\n", size,(int)p);
}

#endif

const char * versionToString(int version)
{
    static char buf[64];
    sprintf(buf, "%d.%02d.%02d",
            version/10000, (version - ((10000 * (version/10000)))) / 100, version % 100);

    return buf;
}

int countChar(const char *s, int c)
{
    int count = 0;
    while (*s)
        if (*s++ == c)
            count++;

    return count;
}

int longestLine (const char *s)
{
    char buf[MAX_MUD_BUF];
    unsigned max_len = 0;
    strcpy(buf, s);

    s = strtok(buf, "\n");
    while (s)
    {
        max_len = max(strlen(s), max_len);
        s = strtok(NULL, "\n");
    }

    return max_len;
}


GlobalStats::GlobalStats() {
    time (&starting_time);
}

static int color_conv_table[8] =  {
    fg_black,
    fg_red,
    fg_green,
    fg_yellow,
    fg_blue,
    fg_magenta,
    fg_cyan,
    fg_white
};

ColorConverter::ColorConverter() : fBold(false), fReport(false), last_fg(fg_white), last_bg(bg_black) {
}

#define MAX_COLOR_BUF 256
int ColorConverter::convert (const unsigned char *s, int size) {
	char    buf[MAX_COLOR_BUF];
	int     code;
	int     color;
	char   *pc;

	if (size < 0 || size >= MAX_COLOR_BUF-1)
		return 0;

	memcpy (buf, s, size);
	buf[size] = NUL;

	if (buf[0] != '\e' || buf[1] != '[')
		return 0;

	pc = buf + 2;

	for (;;) 	{
		code = 0;

		while (isdigit (*pc))
			code = code * 10 + *pc++ - '0';

		switch (code)  {
        case 0:			/* Default */
            fBold = false;
            last_fg = fg_white;
            last_bg = bg_black;
            break;

        case 1:			/* bold ON */
            fBold = true;
            break;

        case 6: 
            if (pc[1] == 'n')
                fReport = true;
            break;

        case 7:			/* reverse */
            /* Ignore for now. It's usually ugly anyway */
            break;

        case 30 ... 37:
        case 40 ... 47:
            if (code <= 37)
                last_fg = (color_conv_table[code - 30]);
            else
                last_bg = (color_conv_table[code - 40]) << 4;

        default:
            ;
        }

		/* Allow for multiple codes separeated with ;s */
		if (*pc != ';')
			break;
		
		pc++;
	}

	color = last_fg | last_bg;
	if (fBold)
		color |= fg_bold;

	/* Suppress black on black */
	if (color == (fg_black|bg_black))
		color |= fg_bold;		

	return color;
}

