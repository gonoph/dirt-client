// Buffer object

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include "defs.h"
#include "Buffer.h"

int Buffer::find_mem_size (int min_size) {
	int i;
	
    for (i = 4096 ; min_size > i; i *= 2)
        ;
    
    return i ;
}

Buffer::Buffer(int min_size) {
    size = find_mem_size(min_size);
    data = new char[size];
    overflowed = false;
    len = 0;
    data[0] = NUL;
}

bool Buffer::strcat (const char *text) {
    return strncat(text, strlen(text));
}

void Buffer::need(int n) {
    if (n > size) { /* expand? */
        int new_size = find_mem_size (n);
        assert (new_size >= 0);
        
        char* new_data = new char[new_size];
        
        memcpy (new_data, data, len);
        delete[] data;
        data = new_data;
        size = new_size;
    }
}

void Buffer::use (int count) {
    assert(count+len <= size);
    len += count;
}

char *Buffer::get (int count) {
    need(count+len+1);
    return data+len;
}

void Buffer::unshift(const char *s, int shifted_len) {
    need(len+shifted_len+1);
    memmove(data,data+len, shifted_len);
    memcpy(data, s, shifted_len);
}


bool Buffer::strncat(const char *text, int text_len) {
	if (overflowed) /* Do not attempt to add anymore if buffer is already overflowed */
		return false;

	if (!text) /* Adding NULL string ? */
		return true;
	
	if (text_len == 0) /* Adding empty string ? */
        return true;

    need(text_len+len+1);
		
	/* Will the combined len of the added text and the current text exceed our buffer? */

	memcpy (data + len, text, text_len);	/* Start copying */
	len += text_len;	/* Adjust length */
	data[len] = NUL; /* Null-terminate at new end */
	
	return true;
	
}

void Buffer::clear() {
    overflowed = false;
    len = 0;
    data[0] = NUL;
}

int Buffer::printf(const char *fmt, ...) {
	char buf[16384];
	va_list va;
	int res;

	va_start (va, fmt);
	res = vsnprintf (buf, sizeof(buf), fmt, va);
	va_end (va);

    if (res >= (int)sizeof(buf)-2)
        abort();
    else
        strcat(buf);

	return res;	
}

void Buffer::shift (int count) {
    assert (count <= len);
    memmove (data, data+count, len-count);
	data[len-count]=0;
    len -= count;
}
