#ifndef __CONFIG_H
#define __CONFIG_H

#define INPUT_SIZE 512

typedef enum {
    opt_histwordsize,
    opt_scrollback_lines,
    opt_histsize,
    opt_showprompt,
    opt_echoinput,
    opt_beep,
    opt_readonly,
    opt_historywindow,
    opt_mudbeep,
    opt_tabsize,
    opt_statcolor,
    opt_inputcolor,
    opt_statuscolor,
    opt_autostatwin,
    opt_autotimerwin,
    opt_speedwalk,
    opt_timercolor,
    opt_timerstate,
    opt_commandcharacter,
    opt_escape_character,
    opt_interpdebug,
    opt_multiinput,
    opt_copyover,
    opt_interpreter_type,
    opt_snarf_prompt,
    opt_undelim_prompt,
    opt_speedwalk_character,
    opt_expand_semicolon,
    opt_plugins,
    opt_chat_name,
    opt_chat_baseport,
    opt_chat_autoaccept,
    opt_chat_download,
    opt_chat_email,
    opt_chat_icon,
    opt_chat_nodisturb,
    opt_chat_interfaces,
    opt_chat_protocol,
    opt_chat_debug,
    opt_chat_paranoia,
    opt_chat_syscolor,
    opt_chat_chatcolor,
    opt_nodefaults,
    opt_save_history,
	max_option
} option_t;

class MUDSelection;

class Macro;

// FIXME why is this extern?
extern class Config {
    public:
    
    Config(const char *fname);
    ~Config();
    MUD * findMud (const char *name);
    
    int parseOptions (int argc, char **argv);	// parse command line options
    
    int getOption (option_t option)	// Get the value of an integer option
    { return options[option]; }

    const String& getStringOption (option_t option) // Get the value of a string option
    { return string_options[option]; } 
	
    void setOption (option_t option, int value)	// Set an option
    { options[option] = value; }

    void setStringOption(option_t option, const char* value)
    {   string_options[option] = value;   }
    
    void compileActions();
    void Save(const char *fname = NULL);
    void parseMUDLine(const char *keyword, const char *buf, MUD *mud);
    void readMUD(FILE *fp, const char *mudname);
    static bool command_writeconfig(string&,void*);

    MUDList *mud_list;
    string filename;		   			// Name of the configuration file

    private:

    void Load(const char *fname);
	
    int options[max_option];
    String string_options[max_option];
    

    friend class MUDSelection;				// messes with mud_list
	
    time_t save_time;					// When was .dirtrc last modifed?

} *config;

#endif
