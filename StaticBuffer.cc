#include "StaticBuffer.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Static buffer code originally by Oliver Jowett

char *getstatic(int len);
void shrinkstatic(char *buf);

StaticBuffer::StaticBuffer (int sz) {
    s = getstatic((len = sz));
}

StaticBuffer::StaticBuffer (const char *buf) {
    s = getstatic((len = strlen(buf+1)));
    strcpy(s,buf);
}

StaticBuffer::~StaticBuffer() {
        shrinkstatic(s);
}

char * StaticBuffer::sprintf(const char *fmt, ...) {
    va_list va;
    StaticBuffer s;
    
    va_start(va, fmt);
    vsnprintf(s, s.size(), fmt, va);
    va_end(va);

    return s;
}

const char* Sprintf(const char *fmt, ...) {
    va_list va;
    StaticBuffer s;
    
    va_start(va, fmt);
    vsnprintf(s, s.size(), fmt, va);
    va_end(va);

    return s;
}

/* Static string buffer allocation */

#define STATIC_SIZE (2 << 16)
static char s_buf[STATIC_SIZE];
static int s_offset;
static char *s_last;

int s_gets;
int s_shrinks;
int s_fails;
int s_size;

char *getstatic(int len)
{
	char *buf;
	
    if (len > STATIC_SIZE)
        abort();
	
	if (s_offset + len >= STATIC_SIZE)
		s_offset = 0;

	buf=&s_buf[s_offset];
	buf[0]=0;
	s_last=buf;

	s_offset += len;
	s_size += len;
	s_gets++;
	
	return buf;
}

void shrinkstatic(char *buf)
{
    int len;
	
	if (!buf)
        return;
	
	s_shrinks++;

	if (buf != s_last)
	{
		s_fails++;
		return;
	}

	len=strlen(buf)+1;

	s_size -= s_last - s_buf - len;
	s_offset = (s_last - s_buf) + len;
}

void resetstatic(void)
{
	s_offset = 0;
	s_last = NULL;
	s_size = 0;
	s_gets = 0;
	s_shrinks = 0;
	s_fails = 0;
}

