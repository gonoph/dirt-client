#include "dirt.h"
#include "cui.h"
#include "Session.h"
#include "Hotkey.h"
#include "Pipe.h"
#include "Interpreter.h"
#include "Curses.h"
#include "EmbeddedInterpreter.h"
#include "Plugin.h"
#include "Chat.h"
#include "Hook.h"
#include <signal.h>
#include <unistd.h>

// Some globals

Config *config;	 // Global configuration FIXME why is this a pointer, and an extern class?
time_t current_time;

GlobalStats globalStats;

Hook hook;
Interpreter interpreter;
StatusLine *status;
MainInputLine *inputLine;
OutputWindow *outputWindow;
Screen *screen;

MUD *lastMud;
Session *currentSession;
TTY *tty;
bool dirtFinished;

InterpreterPipe *interpreterPipe;
OutputPipe *outputPipe;
HistorySet *history;

int session_fd = -1; // Set to somethign else if recovering from copyover

int main(int argc, char **argv) {
    
    // Initialize vcsa screen driver, drop any setgid that we have
    //  Do this right at the start, so that a) perl can startup correctly,
    //  and b) to avoid any unforseen holes in eg. configfile loading     -N
    screen = new Screen();
    screen->name = "screen";
    setegid(getgid());
    
    time (&current_time);
    srand(current_time);
    
    config = new Config(getenv("DIRTRC"));	// Load config file if no $DIRTRC, use .dirt/dirtrc
    history = new HistorySet();
    
    // Parse command line switches: return first non-option	
    int non_option = config->parseOptions(argc, argv);

    // Load the chosen plugins
    Plugin::loadPlugins(config->getStringOption(opt_plugins));
    
    Hotkey* hotkey = new Hotkey(screen);
    hotkey->name = "hotkey";
    
    // Create the output window which will show MUD output
    outputWindow = new OutputWindow(screen);
    outputWindow->name = "outputWindow";

    // Create and insert input line
    // We want this to grow, so it should be on top of the OutputWindow
    inputLine = new MainInputLine();
    inputLine->name = "inputLine";
    
    // Create and insert status line onto screen
    status = new StatusLine(screen);
    status->name = "status";

    
    outputWindow->printVersion();
    
    interpreter.setCommandCharacter((char)config->getOption(opt_commandcharacter));
    history->loadHistory();
    
    // Initialize keyboard driver
    tty = new TTY();
    
    screen->clear();

    // Create pipes to interpreter and screen
    interpreterPipe = new InterpreterPipe;
    outputPipe = new OutputPipe;
    signal(SIGPIPE, SIG_IGN);
    
    // We want error messages from the interpreter to appear on our screen
    // (which is connected to the read end of the STDOUT_FILENO pipe)
    if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
        error ("dup2");
    
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    embed_interp->set("now", current_time);
    embed_interp->set("interpreterPipe", interpreterPipe->getFile(Pipe::Write));
    embed_interp->set("VERSION", versionToString(VERSION));
    embed_interp->set("commandCharacter", Sprintf("%c", config->getOption(opt_commandcharacter)));
    embed_interp->set("DIRT_HOME", INSTALL_ROOT);
    embed_interp->load_file("sys/init");

    if (strlen(config->getStringOption(opt_chat_name)) != 0)
        ChatServerSocket::create(config->getOption(opt_chat_baseport));

    if (argv[non_option]) {
        lastMud = config->findMud(argv[non_option]);
        if (!lastMud) {
            if (argv[non_option+1] && atoi(argv[non_option+1])) {
                // hostname portname. Create a temporary MUD entry
                lastMud = new MUD("temp", argv[non_option], atoi(argv[non_option+1]), &globalMUD);
                config->mud_list->insert(lastMud);
                
                currentSession = new Session(*lastMud, outputWindow, session_fd);
                if (session_fd == -1)
                    currentSession->open();
            }
            status->setf ("MUD %s not in database", argv[non_option]);
        } else {
            currentSession = new Session(*lastMud, outputWindow, session_fd);
            if (session_fd == -1)
                currentSession->open();
        }
    }
    else
        status->setf ("Use %copen (or Alt-O) to connect to a MUD.", interpreter.getCommandCharacter());

    Plugin::displayLoadedPlugins();

    Selectable::select(0, 0); // Allow the embedded interpreters to run once before we run INIT
    interpreter.execute();    // execute any commands the interpreters may have added.
    hook.run(INIT);           // Run INIT to allow scripts to initialize themselves.

    for (dirtFinished = false; !dirtFinished; ) {
        screen->refresh(); // Update the screen here
        
        time (&current_time);
        embed_interp->set("now", current_time);
        
        Selectable::select(1, 250000);
        
//        embed_interp->eval("hook_run('postoutput')", "", NULL);
//        hook.run(POSTOUTPUT, "");
        
        // Execute the stacked commands
        interpreter.execute();
        
        if (currentSession)
            currentSession->idle();  // Some time updates if necessary

        if (chatServerSocket)
            chatServerSocket->idle();
        
        screen->idle();   // Call all idle methods of all windows
        EmbeddedInterpreter::runCallouts();
        
        // This doesn't feel right
        if (currentSession && currentSession->state == disconnected) {
            delete currentSession;
            currentSession = NULL;
            inputLine->set_default_prompt();
            screen->flash();
        }
    }

    set_title("xterm");
//    embed_interp->eval("hook_run('done')", "", NULL);
    hook.run(DONE);
    Plugin::done();
    Selectable::select(0,0); // One last time for anything that was sent by the done scripts
    interpreter.execute();
    
    delete chatServerSocket;
    delete currentSession;      // Close the current session, if any
    delete screen;              // Reset screen (this kills all subwindows too)
//    FIXME the above line causes a crash (on alternate tuesdays).  Probably memory corruption.
    delete tty;                 // Reset TTY state
    delete outputPipe;
    delete interpreterPipe;
    
    freopen("/dev/tty", "r+", stdout);
    freopen("/dev/tty", "r+", stderr);
    
    fprintf (stderr, CLEAR_SCREEN
             "You wasted %d seconds, sent %d bytes and received %d.\n"
             "%ld bytes of compressed data expanded to %ld bytes (%.1f%%)\n"
             "%d characters written to the TTY (%d control characters).\n"
             "Goodbye!\n",
             (int)(current_time - globalStats.starting_time),
             globalStats.bytes_written,
             globalStats.bytes_read,
             globalStats.comp_read,
             globalStats.uncomp_read,
             globalStats.uncomp_read ? (float)globalStats.comp_read / (float)globalStats.uncomp_read * 100.0 : 0.0,
             globalStats.tty_chars, globalStats.ctrl_chars
            );

    history->saveHistory();// OJ: better do this before config is deleted..
    delete config;     // Save configuration; updates stats too
    
    return 0;
}

