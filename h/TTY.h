// TTY. handles reading keys from the keyboard
// Control keys are parsed here

class TTY : public Selectable
{
public:
    TTY();               // Init TTY stuff, switch to single char
    ~TTY();							 // Back to linemode
    
    virtual int init_fdset(fd_set*, fd_set*);
    virtual void check_fdset(fd_set*, fd_set*);
    
    void suspend();					// Prepare for suspension; go back to linemode
    void restore();					// Recover from suspension; go to raw

    time_t last_activity;           // Last time we pressed anything
private:
    struct termios *old_term;
    int get_raw_key();				// Read one character
    int get_key();					// Read a key. translate metakeys
    void set_normal();
    void set_raw();
    void init_keys();

};

extern TTY *tty;

// Keyboard codes
enum
{
    key_none = -1,
    key_ctrl_a = 1,
    key_ctrl_b,
    key_ctrl_c,
    key_ctrl_d,
    key_ctrl_e,
    key_ctrl_f,
    key_ctrl_g,
    key_ctrl_h,
    key_tab	   = 9,
    key_ctrl_i = key_tab,
    key_ctrl_j = 10,
    key_ctrl_k,
    key_ctrl_l,
    keyEnter,
    key_ctrl_n,
    key_ctrl_o,
    key_ctrl_p,
    key_ctrl_q,
    key_ctrl_r,
    key_ctrl_s,
    key_ctrl_t,
    key_ctrl_u,
    key_ctrl_v,
    key_ctrl_w,
    key_ctrl_x,
    key_ctrl_y,
    key_ctrl_z,
    
    key_backspace = 0x7f,
    
    // Here are the normal keycodes
    
    key_arrow_up = 256,
    key_arrow_down,
    key_arrow_right,
    key_arrow_left,
    key_page_up,
    key_page_down,
    key_end,
    key_home,
    key_insert,
    key_delete,
    key_pause,
    key_escape,

    key_alt_1,  key_alt_2,  key_alt_3,  key_alt_4,  key_alt_5,
    key_alt_6,  key_alt_7,  key_alt_8,  key_alt_9,  key_alt_0,
    
    key_alt_a, 	key_alt_b, 	key_alt_c, 	key_alt_d, 	key_alt_e,
    key_alt_f, 	key_alt_g, 	key_alt_h, 	key_alt_i, 	key_alt_j,
    key_alt_k, 	key_alt_l, 	key_alt_m, 	key_alt_n, 	key_alt_o,
    key_alt_p, 	key_alt_q, 	key_alt_r, 	key_alt_s, 	key_alt_t,
    key_alt_u, 	key_alt_v, 	key_alt_w, 	key_alt_x, 	key_alt_y,
    key_alt_z,
    
    key_kp_1,	key_kp_2,	key_kp_3,	key_kp_4,
    key_kp_5,	key_kp_6,	key_kp_7,	key_kp_8,
    key_kp_9,	key_kp_0,	
    key_kp_plus,	key_kp_minus,	key_kp_divide,	key_kp_multi,
    key_kp_enter,	key_kp_numlock,	key_kp_period,
    
    key_f1,		key_f2,		key_f3,		key_f4,		key_f5,
    key_f6,		key_f7,		key_f8,		key_f9,		key_f10,
    key_f11,	key_f12,
    
    key_alt_slash,
};

const char *key_name (int key);
int key_lookup (const char *name);



