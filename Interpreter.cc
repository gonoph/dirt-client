// a class to keep a list of commands and execute them one at a time

#include "dirt.h"
#include "cui.h"
#include "Option.h"
#include "MessageWindow.h"
#include "Interpreter.h"
#include "Shell.h"
#include "Action.h"
#include "Alias.h"
#include "Chat.h"
#include "Session.h"
#include "MUD.h"
#include "Hook.h"

extern MUD *lastMud;

bool actions_disabled, aliases_disabled, macros_disabled;
// Pick off one argument, optionally smashing case and place it in buf
// Eat whitespace, respect "" and ''
// FIXME This fails for escaped quotes.  From Notes:
// Add a function to take the command, parse it, and return argv, argc style arg
//    list?  Should I do option processing too?  (would add to consistency)
//    Maybe return a hash of option->value pairs.
const char *one_argument (const char *argument, char *buf, bool smash_case) {
    char end;
    
    while(isspace(*argument))
        argument++;

    if (*argument == '\'' || *argument == '\"')
        end = *argument++;
    else
        end = NUL;

    while(*argument && (end ? *argument != end : !isspace(*argument)))  {
        *buf++ = smash_case ? tolower(*argument) : *argument;
        argument++;
    }

    *buf++ = NUL;

    if (*argument && end)
        argument++;

    while(isspace(*argument))
        argument++;

    return argument;
}

bool Interpreter::command_quit(string&, void*) {
    dirtFinished = true;
    return true;
}

bool Interpreter::command_echo(string& str, void*) {
    OptionParser opt(str, "nW:");
    string s = opt.restStr();
    if(!opt.flag('n')) s.append("\n");

    if(!opt.valid()) return true;
    if(opt.gotOpt('W')) {
        MessageWindow* w = MessageWindow::find(opt['W'].c_str());
        if (!w) {
            status->setf("%cprint: %s: no such window", CMDCHAR, opt['W'].c_str());
            return true;
        }
        w->addInput(s.c_str());
    } else outputWindow->printf("%s", s.c_str());
    return true;
}

bool Interpreter::command_status(string& str, void*) {
    OptionParser opt(str, "W:");
    if(!opt.valid()) return true;
    if(!opt.gotOpt('W')) status->setf("%s", opt.restStr().c_str());
    else {
        MessageWindow* mw = MessageWindow::find(opt['W']);
        if(!mw) { 
            report_err("%cstatus: unable to find window '%s'\n", CMDCHAR, opt['W'].c_str()); 
            return true;
        }
        mw->set_bottom_message(opt.restStr().c_str());
    }
    return true;
}

bool Interpreter::command_bell(string&, void*) {
    screen->flash();
    return true;
}

bool Interpreter::command_exec(string& str, void*) {
    int w = 80, h=10, x=0, y=3, t=10;
    
    OptionParser opt(str, "w:h:x:y:t:");
    if(!opt.valid()) return true;
    
    if(opt.gotOpt('w')) w = atoi(opt['w'].c_str());
    if(opt.gotOpt('h')) h = atoi(opt['h'].c_str());
    if(opt.gotOpt('x')) x = atoi(opt['x'].c_str());
    if(opt.gotOpt('y')) y = atoi(opt['y'].c_str());
    if(opt.gotOpt('t')) t = atoi(opt['t'].c_str());
            
    // Do some sanity checking
    if (h < 3)
        status->setf("%cexec: Value h=%d too low.", CMDCHAR, h);
    else if (w < 3)
        status->setf("%cexec: Value w=%d too low.", CMDCHAR, w);
    else if (x < 0)
        status->setf("%cexec: Value x=%d too low.", CMDCHAR, x);
    else if (y < 0)
        status->setf("%cexec: Value y=%d too low.", CMDCHAR, y);
    else if (w+x > screen->width)
        status->setf("%cexec: Value w=%d or x=%d too large.  Window would extend beyond screen.", CMDCHAR, w, x);
    else if (y+h > screen->height)
        status->setf("%cexec: Value y=%d or h=%d too large.  Window would extend beyond screen.", CMDCHAR, y, h);
    else
        new Shell(screen, opt.restStr().c_str(), w,h,x,y,t);
    return true; 
}

bool Interpreter::command_clear(string& str, void*) {
    OptionParser opt(str, "W:");
    if(!opt.valid()) return true;
    string name;
    if(opt.gotOpt('W')) name = opt['W'];
    else name = opt.arg(1);
    
    if (!name.length()) {  // no window specified.
        outputWindow->clear();
        outputWindow->move(0,0);
    }
    else {
        MessageWindow *w = MessageWindow::find(name);
        if (!w)
            status->setf("%cclear: %s: no such window", CMDCHAR, name.c_str());
        else {
            w->clear();
            w->gotoxy(0,0);
        }
    }
    return true;
}

bool Interpreter::command_prompt(string& str, void*) {
    OptionParser opt(str, "");
    if(!opt.valid()) return true;
    inputLine->set_prompt(opt.restStr().c_str());
    return true;
}

bool Interpreter::command_close(string&, void*) {
    if (!currentSession || currentSession->state == disconnected)
        status->setf ("You are not connected - nothing to close");
    else
    {
        status->setf ("Closing link to %s@%s:%d", ~currentSession->mud.name, currentSession->mud.getHostname(), currentSession->mud.getPort());
        inputLine->set_default_prompt();
        delete currentSession;
        currentSession = NULL;
    }
    return true;
}

bool Interpreter::command_open(string& str, void*) {
    OptionParser opt(str, "");
    if(!opt.valid()) return true;
    string name = opt.arg(1);
    if(opt.argc() < 1) status->setf ("%copen: open a connection to where?", CMDCHAR);

    if (currentSession && currentSession->state != disconnected)
        status->setf ("You are connected to %s, use %cclose first", ~currentSession->mud.name, CMDCHAR);
    else
    {
        MUD *mud;
        if (!(mud = config->findMud (name.c_str())))
            status->setf ("MUD '%s' was not found", name.c_str());
        else
        {
            delete currentSession;
            currentSession = new Session(*mud, outputWindow);
            currentSession->open();
            lastMud = mud;
        }
    }
    return true;
}

bool Interpreter::command_reopen(string&, void* mt) {
    Interpreter* mythis = (Interpreter*)mt;
    if (!lastMud)
        status->setf ("There is no previous MUD to which I can reconnect");
    else
    {
        string s = "/open ";
        s += ~lastMud->name;
        mythis->commands.push_back(s);
    }
    return true;
}

bool Interpreter::command_send(string& str, void*) {
    OptionParser opt(str, "nu");
    if(!opt.valid()) return true;
    string towrite = opt.restStr();

    if (!currentSession) status->setf("No session active");
    else {
        if(opt.gotOpt('n')) {
            if(opt.gotOpt('u')) 
                currentSession->writeUnbuffered(towrite.c_str(), towrite.length());
            else writeMUDn(towrite);
        }
        else { 
            string gonnawrite = towrite.append("\n");
            if(opt.gotOpt('u')) 
                currentSession->writeUnbuffered(gonnawrite.c_str(), gonnawrite.length());
            else writeMUD(towrite);  // writeMUD adds a CR
        }
    }
    return true;
}

bool Interpreter::command_setinput(string& str, void*) {
    inputLine->set(str.c_str());
    return true;
}

bool Interpreter::command_save(string& str, void*) {
    OptionParser opt(str, "a");
    if(!opt.valid()) return true;
    bool color = opt.gotOpt('a');
    
    if(opt.argc() < 1) status->setf("Specify file to save scrollback to.");
    else outputWindow->saveToFile(opt.arg(1).c_str(), color);
    return true;
}

// Construct Interpreter, add dirt commands as command hook callbacks.
Interpreter::Interpreter() {    
    hook.addDirtCommand("quit", &command_quit, (void*)this);        /* doc */
    hook.addDirtCommand("echo", &command_echo, (void*)this);        /* doc */
    hook.addDirtCommand("status", &command_status, (void*)this);    /* doc */
    hook.addDirtCommand("bell", &command_bell, (void*)this);        /* doc */
    hook.addDirtCommand("exec", &command_exec, (void*)this);        /* doc */
    hook.addDirtCommand("clear", &command_clear, (void*)this);      /* doc */
    hook.addDirtCommand("prompt", &command_prompt, (void*)this);    /* doc */
    hook.addDirtCommand("close", &command_close, (void*)this);      /* doc */
    hook.addDirtCommand("open", &command_open, (void*)this);        /* doc */
    hook.addDirtCommand("reopen", &command_reopen, (void*)this);    /* doc */ // FIXME make an alias instead?
    hook.addDirtCommand("send", &command_send, (void*)this);        /* doc */
    hook.addDirtCommand("setinput", &command_setinput, (void*)this);/* doc */
    hook.addDirtCommand("save", &command_save, (void*)this);        /* doc */
    hook.addDirtCommand("window", &MessageWindow::command_window, (void*)NULL); /* doc */
    hook.addDirtCommand("hook", &Hook::command_hook, (void*)&hook); /* doc */     // FIXME unfinished...
    hook.addDirtCommand("enable", &Hook::command_enable, (void*)&hook); /* doc */
    hook.addDirtCommand("disable", &Hook::command_disable, (void*)&hook); /* doc */
    hook.addDirtCommand("group", &Hook::command_group, (void*)&hook); /* doc */
    hook.addDirtCommand("run", &EmbeddedInterpreter::command_run, (void*)embed_interp);  /* doc */
    hook.addDirtCommand("eval", &EmbeddedInterpreter::command_eval, (void*)embed_interp);/* doc */
    hook.addDirtCommand("load", &EmbeddedInterpreter::command_load, (void*)embed_interp);/* doc */
    hook.add(SEND, new CppHookStub(INT_MAX, 1.0, -1, false, true, true,  // FIXME command to toggle this?
        "__DIRT_BUILTIN_expandSemicolon", vector<string>(1,"Dirt builtins"), &Interpreter::expandSemicolon));
    hook.add(SEND, new CppHookStub(INT_MAX-1, 1.0, -1, false, true, true,// FIXME command to toggle this?
        "__DIRT_BUILTIN_expandSpeedwalk", vector<string>(1,"Dirt builtins"), &Interpreter::expandSpeedwalk));
}

void Interpreter::execute() {
    // FIXME should we process a fixed # of commands and return?  We can always
    // process the rest on the next call, and we need to give the screen a chance
    // to update if we're sending lots of commands.
    while(commands.size())    
    {
        string line = commands.front();
        commands.pop_front();  // destroy the command on the top of the stack.
//        report("Interpreter::execute executing command '%s'\n", line.c_str());
        
        if (line.length() > 0 && line[0] == commandCharacter) {
            hook.run(COMMAND, line);
            dirtCommand (line.c_str() + 1);  // FIXME remove this once all commands are class members
        } else if(currentSession) {
            hook.run(SEND, line);
        } else
            status->setf ("You are not connected. Use Alt-O to connect to a MUD.");
    }
}

// Legal speedwalk strings
const char legal_standard_speedwalk[] = "nsewud0123456789";
const char legal_extended_speedwalk[] = "nsewudhjkl0123456789";

// Expand possible speedwalk string
bool Interpreter::expandSpeedwalk(string& str)
{
    unsigned int start = 0;
    bool try_speedwalk = config->getOption(opt_speedwalk);
    
    if (str[0] == config->getOption(opt_speedwalk_character)
        && str.find_first_not_of(legal_extended_speedwalk, 1) == str.npos) {
        start = 1;
        try_speedwalk = true;
    } else if(str.find_first_not_of(legal_standard_speedwalk) == str.npos) {
        start = 0;
    } else {
        try_speedwalk = false;
    }
    
    if (try_speedwalk)
    {
        int repeat = 0,j = 0;
        deque<string> replacement;
        for(unsigned int i=start;i<str.length();i++) {
            switch (str[i]) {
                case 'u':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("u"); 
                    repeat=0;
                    break;
                case 'd':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("d"); 
                    repeat=0;
                    break;
                case 'n':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("n"); 
                    repeat=0;
                    break;
                case 's':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("s"); 
                    repeat=0;
                    break;
                case 'e':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("e"); 
                    repeat=0;
                    break;
                case 'w':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("w"); 
                    repeat=0;
                    break;
// extra cases won't matter for standard speedwalk since we already checked that
// the string doesn't contain hjkl
                case 'h':   
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("nw"); 
                    repeat=0;
                    break;
                case 'j':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("ne"); 
                    repeat=0;
                    break;
                case 'k':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("sw"); 
                    repeat=0;
                    break;
                case 'l':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back("se"); 
                    repeat=0;
                    break;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    repeat = 10*repeat + str[i] - '0';
            }
        }
        interpreter.commands.insert(interpreter.commands.begin(), replacement.begin(), 
            replacement.end());
        return true;  // non-fallthrough will prevent other hooks from processing this command
    }
    return false; // not a speedwalk string, will fall-through to other hooks.
}

bool Interpreter::expandSemicolon(string& str) {
    bool retval = false;
    size_t pos=0, lastescape=0, firstescape=0;
    deque<string> replacement;

    while((pos = str.find(';', pos)) != string::npos) { // returns index of first ; or -1 if not found
        if(str[pos-1] == '\\') {
            lastescape = str.rfind('\\', pos);
            lastescape = (lastescape > 0)?lastescape:pos;
            firstescape = str.find_last_not_of('\\', pos-1) + 1;
            if((pos - firstescape)%2 == 0) {   // number of backslashes is even.
                replacement.push_back(str.substr(0, pos));
                str.replace(0, pos, "");
                retval = true;
                pos = 0;
            } else { // number of backslashes is odd, semicolon is escaped.  (DO NOTHING...)
                pos++; // keep going...(debackslashify in writeMUD will take care of backslashes)
            }
        } else {
            replacement.push_back(str.substr(0, pos));
            str.replace(0, pos + 1, "");
            pos = 0;
            retval = true;
        }
    }
    if(retval) {
        replacement.push_back(str);
    }
    interpreter.commands.insert(interpreter.commands.begin(), replacement.begin(), 
        replacement.end());
    return retval;
}

// Execute a dirt Command (s should *not* start with commandCharacter)
void Interpreter::dirtCommand (const char *s)
{
    char cmd[256];
    
    s = one_argument(s, cmd, true);
    
    if (!strcmp(cmd, "alias")) // add alias
    {
        char name[MAX_INPUT_BUF];
        
        if (!s[0])
            status->setf ("Press Alt-A to list active aliases; Alt-O then A to list local ones");
        else 
        {
            s = one_argument(s, name, false);
            Alias *a = globalMUD.findAlias(name);
            
            if (!s[0]) {
                if (!a)
                    status->setf ("There is no global alias named '%s'", name);
                else  {
                    status->setf ("Removed global alias '%s'", name);
                    globalMUD.alias_list.erase(name);
                    delete a;
                }
            } else {
                if (a) {
                    status->setf ("Replaced alias '%s' with %s", a->name.c_str(), s);
                    a->text = s;
                } else {
                    status->setf ("Added new alias %s = %s", name,s);
                    globalMUD.alias_list[name] = new Alias(name,s);
                }
            }
        }
    }
    else if (!strcmp(cmd, "macro")) {
        char name[MAX_INPUT_BUF];
        Macro *m;
        
        s = one_argument(s, name, true);
        int key = key_lookup(name);
        
        if (key < 0)
            status->setf ("Unknown key in macro definition: %s", name);
        else {
            m = globalMUD.findMacro(key, false);
            
            if (!s[0]) { // remove macro
                if (!m)
                    status->setf("No such macro");
                else {
                    globalMUD.macro_list.remove(m);
                    status->setf("Removed macro %s => %s", name, ~m->text);
                    delete m;
                }
            } else { // Add new or change existing
                if (!m) {
                    m = new Macro(key, s);
                    status->setf("Added new macro: %s => %s", name,s);
                    globalMUD.macro_list.append(m);
                } else {
                    status->setf("Changed macro %s: from %s to %s", name, ~m->text, s);
                    m->text = s;
                }
            }
        }
    }
    else if (!strcmp(cmd, "action") || !strcmp(cmd, "subst"))
    {
        char name[256];
        s = one_argument(s, name, false);
        
        if (!name[0])
            status->setf ("Add/remove what action?");
        else if (!s[0]) {
            FOREACH(Action*, a, globalMUD.action_list)
                if (a->pattern == name)
                {
                    status->setf ("Action %s => %s removed", (const char*) a->pattern, (const char *) a->commands);
                    globalMUD.action_list.remove(a);
                    delete a;
                    return;
                }
            
            status->setf("Action '%s' was not found", name);
        } else {
            char buf[MAX_MUD_BUF];
            String res;
            
            sprintf(buf, "\"%s\" %s", name, s);
            
            Action *new_action = Action::parse (buf, res, (!strcmp(cmd, "action")) ? Action::Trigger : Action::Replacement);
            if (!new_action)
                status->setf ("Error compiling action: %s", ~res);
            else {
                FOREACH(Action*,a, globalMUD.action_list)
                    if (a->pattern == name)  {
                        status->setf ("Replaced action '%s' with '%s'", (const char*) new_action->pattern, (const char *) new_action->commands);
                        globalMUD.action_list.remove(a);
                        delete a;
                        globalMUD.action_list.insert(new_action);
                        return;
                    }
                
                globalMUD.action_list.insert(new_action);
                status->setf ("Added action '%s' => '%s'", (const char*) new_action->pattern, (const char *) new_action->commands);
            }
        }
    }
    
    // Chat subcommands
    else if (!strncmp(cmd, "chat.", strlen("chat."))) {
        const char *subcmd = cmd+strlen("chat.");
        if (!chatServerSocket)
            status->setf("CHAT is not active. Set chat.name option in the config file");
        else
            chatServerSocket->handleUserCommand(subcmd,s);
    }

    // Chat aliases
    else if (!strcmp(cmd, "chat"))
        dirtCommand(Sprintf("%s %s", "chat.to", s));
    else if (!strcmp(cmd, "chatall"))
        dirtCommand(Sprintf("%s %s", "chat.all", s));
    else {
        if (embed_interp->run_quietly(NULL, Sprintf("dirtcmd_%s", cmd),s, NULL, true))
            return;
        
//        status->setf ("Unknown command: %s", cmd);
    }
}

void Interpreter::setCommandCharacter(int c) {
  commandCharacter = c;
}

char Interpreter::getCommandCharacter() {
  return commandCharacter;
}

