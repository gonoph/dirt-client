#ifndef DIRT_H_
#define DIRT_H_

#include "defs.h"

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <string>
#include <hash_map>

extern time_t current_time;
extern char* tmpbuf;  // temporary buffer for everyone's use.

// Define to 1) print all allocs to stderr
// 2) put a magic marker before and after all memory allocated
// #define MEMORY_DEBUG
#ifdef MEMORY_DEBUG
void * operator new (size_t size);
void operator delete (void *ptr);
#endif

inline int max(int a, int b) {
	return a > b ? a : b;
}

inline int min(int a, int b) {
	return a < b ? a : b;
}

// This is not provided by g++ ... it should be.
template <>     // create a hashing algorithm for string that uses the const char* hashing algo.
class hash<string>
{
public:
    inline size_t operator()(string const &str) const {
        hash<char const *> h;
        return (h(str.c_str()));
    }
};

// Ought to split it up
#include "Color.h"
#include "List.h"
#include "Selectable.h"
#include "String.h"
#include "MUD.h"
#include "Config.h"
#include "Buffer.h"
#include "TTY.h"
#include "config.h"
#include "misc.h"
#include "global.h"
#include "EmbeddedInterpreter.h"
#include "StaticBuffer.h"

extern EmbeddedInterpreter *embed_interp;
extern const char *szDefaultPrompt;
extern bool dirtFinished;

extern struct GlobalStats {
     int tty_chars;
     int ctrl_chars;
     int bytes_written;
     int bytes_read;
     time_t starting_time;
     unsigned long comp_read;
     unsigned long uncomp_read;
     
     GlobalStats();
} globalStats;

#define CMDCHAR config->getOption(opt_commandcharacter)

// Search for stuff there
#define DIRT_LOCAL_LIBRARY_PATH "/usr/local/lib/dirt"
#define DIRT_LIBRARY_PATH "/usr/lib/dirt"

#define CLEAR_SCREEN "\ec\e[0;0m\e[H\e[J"
#endif
