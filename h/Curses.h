void init_curses(bool);
void set_title(const char*); // set xterm title
const char *lookup_key(const char *); // kf1-> whatver f1 key should return

extern char special_chars[];
extern char real_special_chars[];

enum {
    bc_vertical, bc_horizontal, bc_upper_left, bc_lower_left,
    bc_upper_right, bc_lower_right,
    sc_filled_box,
    sc_half_filled_box,
    max_sc
};

extern const char *smacs, *rmacs; // set/reset ACS
