#ifndef DIRT_GLOBALSTATS_H
#define DIRT_GLOBALSTATS_H

#include <sys/types.h>


struct GlobalStats {
    int tty_chars;
    int ctrl_chars;
    int bytes_written;
    int bytes_read;
    time_t starting_time;
    unsigned long comp_read;
    unsigned long uncomp_read;

    GlobalStats();
};

extern GlobalStats globalStats;





#endif
