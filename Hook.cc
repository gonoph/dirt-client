/* Hook.cc
 * 
 * 2/10/2001 Bob McElrath
 * 
 * Hooks are classes of callbacks.  In other words, you can register a function
 * to be executed when a hook is called.  The hook 'output' for instance, will
 * be called each time there is a line of output from the mud.  A function
 * registered as an 'output hook' will get called once for each line of output.
 */

#include "Hook.h"
#include "misc.h"
#include "TTY.h"
#include "Interpreter.h"
#include "EmbeddedInterpreter.h"
#include "Option.h"
#include "Config.h"
#include "MessageWindow.h"
#include "InputLine.h"
#include "Pipe.h"

#include <iomanip>
#include <algorithm>  // min(), max()

HookStub::HookStub(int p, float c, int n, bool F, bool en, bool col, string nm, vector<string> g) 
    : priority(p), chance(c), shots(n), fallthrough(F), enabled(en), color(col), name(nm), groups(g) 
{}

Hook::Hook() : max_type(IDLE), hooks(), types() {
// I think the hash_map is case insensitive?!?!?!?
    /*types["command"]   = types["Command"]   =*/ types["COMMAND"]   = COMMAND;
    /*types["userinput"] = types["Userinput"] =*/ types["USERINPUT"] = USERINPUT;
    /*types["send"]      = types["Send"]      =*/ types["SEND"]      = SEND;
    /*types["output"]    = types["Output"]    =*/ types["OUTPUT"]    = OUTPUT;
    /*types["loselink"]  = types["Loselink"]  =*/ types["LOSELINK"]  = LOSELINK;
    /*types["prompt"]    = types["Prompt"]    =*/ types["PROMPT"]    = PROMPT;
    /*types["init"]      = types["Init"]      =*/ types["INIT"]      = INIT;
    /*types["done"]      = types["Done"]      =*/ types["DONE"]      = DONE;
    /*types["keypress"]  = types["Keypress"]  =*/ types["KEYPRESS"]  = KEYPRESS;
    /*types["connect"]   = types["Connect"]   =*/ types["CONNECT"]   = CONNECT;
    /*types["idle"]      = types["Idle"]      =*/ types["IDLE"]      = IDLE;
    for(int i=0;i<=max_type;i++) {
        hooks.push_back(new hookstubset_type());
    }
}

bool Hook::command_hook(string& s, void* mt, savedmatch*) {
    Hook* mythis = (Hook*)mt;
    OptionParser opt(s, "aFilDt:p:c:n:g:T:C:r:N:d:k:W:");
    if(!opt.valid()) {
        // OptionParser will print the error message for us.
        return true;
    }
    int   priority    = opt.gotOpt('p')?atoi(opt['p'].c_str()):0;
    float chance      = opt.gotOpt('c')?atof(opt['c'].c_str()):1.0;
    int   nshots      = opt.gotOpt('n')?atoi(opt['n'].c_str()):-1;
    bool  fallthrough = opt.gotOpt('F');
    bool  enabled     = !opt.gotOpt('D');
    bool  wantansi    = opt.gotOpt('a');
    int   key         = opt.gotOpt('k')?key_lookup(opt['k'].c_str()):-1;
    vector<string> groups;
    if(opt.gotOpt('g')) { // Multiple groups may be separated by commas
        size_t pos = 0, lastpos = 0;
        while((pos = opt['g'].find(',', pos)) != string::npos) {
            groups.push_back(opt['g'].substr(lastpos, pos-lastpos));
            pos++;
            lastpos = pos;
        }
        groups.push_back(opt['g'].substr(lastpos, opt['g'].length()-lastpos));
    }
    string trigstr;
    string window    = opt.gotOpt('W')?opt['W']:"";

    HookType type;
    if(opt.gotOpt('T')) {
	int CmdChar = config->getOption(opt_commandcharacter);
        if(mythis->types.find(opt['T']) == mythis->types.end()) {
            report_err("%chook: unknown type: %s\n", CmdChar, opt['T'].c_str());
            return true;
        } else type = mythis->types[opt['T']];
        if(opt.gotOpt('k') && type != KEYPRESS) {
            report_err("%chook: -k doesn't make sense with -T %s\n", CmdChar, opt['T'].c_str());
            return true;
        }
    } else {
        if(opt.gotOpt('k')) type = KEYPRESS;
        else if(opt.gotOpt('C')) type = COMMAND;
        else if(opt.gotOpt('t')) type = OUTPUT;
        else type = SEND;
    }
    
    int CmdChar = config->getOption(opt_commandcharacter);
    if(opt.gotOpt('l')) {
        if(opt.argc() < 2) { // 2 args means there's no = and no definition.
            bool showbuiltin = opt.gotOpt('i');
            report("%-35s%11s%10s%7s%6s%6s\n", "Hook name", "Priority", "Type", "Chance", "Shots", "Flags");
            for(hooknames_type::iterator it=mythis->hooknames.begin();it != mythis->hooknames.end();it++) {
                if(!showbuiltin && it->first.substr(0, 6) == "__DIRT") continue;
                if(opt.gotOpt('T') && type != it->second->type) continue;
                // Print a symbolic name for the type.
                for(types_type::iterator tit = mythis->types.begin(); tit != mythis->types.end(); tit++) {
                    if(it->second->type == tit->second) {
                        report("%-35s%11d%10s %5.1f%%%6d%2s%2s%2s\n", it->first.c_str(), 
                            it->second->priority, tit->first.c_str(), it->second->chance*100,
                            it->second->shots, it->second->fallthrough?"F":"", 
                            it->second->enabled?"":"D", it->second->color?"C":"");
                    }
                }
            }
            return true;
        } else { // We were passed the name of a hook
            int i=0;
            while(opt.arg(i++).length()) {
                if(mythis->hooknames.find(opt.arg(i)) != mythis->hooknames.end()) {
                    HookStub* stub = mythis->hooknames[opt.arg(i)];
                    hooknames_type::iterator it = mythis->hooknames.find(opt.arg(i));
                    for(types_type::iterator tit = mythis->types.begin(); tit != mythis->types.end(); tit++) {
                        if(stub->type == tit->second) {
                            report("%-35s%11d%10s %5.1f%%%6d%2s%2s%2s\n", 
                                it->first.c_str(), 
                                stub->priority, tit->first.c_str(), stub->chance*100, stub->shots,
                                stub->fallthrough?"F":"", stub->enabled?"":"D", stub->color?"C":"");
                            report("Groups: \n");
                            for(vector<string>::iterator git = stub->groups.begin();
                                    git != stub->groups.end();git++) {
                                report("\t%s\n", git->c_str());
                            }
                            mythis->hooknames[opt.arg(i)]->print();
                            // Print TriggerHookStub::command,matcher,language
                        }
                    }
                }
            }
            return true;
        }
    } else if(opt.gotOpt('t') && opt.gotOpt('C')) { // Trigger regex
        report_err("%chook: Options -t and -C are mutually exclusive.", CmdChar);
    } else if(opt.gotOpt('t') && type == COMMAND) {
        report_err("%chook: -t option is incompatible with COMMAND type hook.", CmdChar);
    } else if(opt.gotOpt('t')) {
        trigstr = opt['t'];
    } else if(opt.gotOpt('C')) {
        trigstr = "^";
        if(type == COMMAND) {
            char c = interpreter.getCommandCharacter();
            trigstr.append(1, c);
        }
        trigstr.append(opt['C']);
        trigstr.append("(?:(?=\\s)|$)");  // zero-width positive lookahead -- next char must be a space.
                                          // or end of string
    } else if(opt.gotOpt('r')) {
        mythis->run(type, opt['r']);
        return true;
    } else if(opt.gotOpt('N')) {
        if(mythis->types.find(opt['N']) == mythis->types.end()) {
            mythis->add_type(opt['N']);
//            report("%chook: Added new hook type %s", CMDCHAR, opt['N'].c_str());
        } else {
            report_err("%chook: Hook type %s already defined!\n", CmdChar, opt['N'].c_str());
        }
        return true;
    } else if(opt.gotOpt('d')) {
        if(mythis->hooknames.find(opt['d']) != mythis->hooknames.end()) {
            mythis->remove(opt['d']);
//            report("Removed hook %s.\n", opt['d'].c_str());
        } else {
            report_err("%chook: Hook %s is not defined.\n", CmdChar, opt['d'].c_str());
        }
        return true;
    } else { // This is a definition, -t was not specified, assume empty regex.
        trigstr = "";
    }
    if(opt.argc() < 4) {
        report_err("%chook: Not enough arguments in command:",CmdChar);
        report_err("\t%s", s.c_str());
        return true;
    }
    string name      = opt.arg(1);
    string equals    = opt.arg(2);
    string action;
    if(opt.argc() != 4) {
        action = opt.restStr();
        size_t eqpos     = action.find("=");
        if(equals.compare("=") != 0 && eqpos != string::npos) {
            report_err("%chook: invalid syntax (equal sign not found) in command:\n", CmdChar);
            report_err("\t%s", s.c_str());
            return true;
        }
        size_t actpos = action.find_first_not_of(" \t\n", eqpos+1);
        if(actpos == string::npos) {
            report_err("%chook: Unable to find action!\n", CmdChar);
            return true;
        }
        action = action.substr(actpos, action.length()-actpos);
    } else action = opt.arg(3);

    HookStub* stub = NULL;
    if(opt.gotOpt('k')) { // KEYPRESS hook, use KeypressHookStub
        stub = new KeypressHookStub(priority, chance, nshots, fallthrough, enabled, wantansi, name,
            groups, trigstr, action, key, window);
    } else { // regular hook
        stub = new TriggerHookStub(priority, chance, nshots, fallthrough, enabled, wantansi, name,
            groups, trigstr, action);
    }
    if(stub == NULL) {
        report_err("%chook: 'new' failed to create new HookStub for hook '%s'!\n", CmdChar, name.c_str());
    } else {
        hook.add(type, stub);
    }

//    report("Added hook %s with priority %d\n", name.c_str(), priority);
    return true;
}

bool Hook::command_disable(string& s, void* mt, savedmatch*) {
    Hook* mythis = (Hook*)mt;
    OptionParser opt(s, "g");

    if(!opt.valid()) return true;
    if(opt.argc() < 2) {
        int CmdChar = config->getOption(opt_commandcharacter);
	report("%cdisable: need the name of a hook to disable\n", CmdChar);
        return true;
    }
    if(opt.gotOpt('g')) for(int i=1;i<opt.argc();i++) mythis->disableGroup(opt.arg(i));
    else for(int i=1;i<opt.argc();i++) {
        if(mythis->hooknames.find(opt.arg(i)) != mythis->hooknames.end())
            mythis->disable(opt.arg(i));
        else 
            report_err("/disable: '%s' is not a defined hook!\n", opt.arg(i).c_str());
    }
    return true;
}

bool Hook::command_enable(string& s, void* mt, savedmatch*) {
    Hook* mythis = (Hook*)mt;
    OptionParser opt(s, "g");

    if(!opt.valid()) return true;
    if(opt.argc() < 2) {
        report("%cenable: need the name of a hook to disable\n", interpreter.getCommandCharacter());
        return true;
    }
    if(opt.gotOpt('g')) for(int i=1;i<opt.argc();i++) 
        mythis->enableGroup(opt.arg(i));
    else for(int i=1;i<opt.argc();i++) {
        if(mythis->hooknames.find(opt.arg(i)) != mythis->hooknames.end())
            mythis->enable(opt.arg(i));
        else 
            report_err("/enable: '%s' is not a defined hook!\n", opt.arg(i).c_str());
    }
    return true;
}

bool Hook::command_group(string& s, void* mt, savedmatch*) {
    Hook* mythis = (Hook*)mt;
    OptionParser opt(s, "l:");
    int CmdChar = config->getOption(opt_commandcharacter);

    if(!opt.valid()) return true;
    if(opt.gotOpt('l')) {
        if(mythis->hookgroups.find(opt['l']) != mythis->hookgroups.end()) {
            report("%-35s%11s%10s%7s%6s%6s\n", "Hook name", "Priority", "Type", "Chance", "Shots", "Flags");
            for(vector<HookStub*>::iterator it = mythis->hookgroups[opt['l']].begin();it != mythis->hookgroups[opt['l']].end(); it++) {
                for(types_type::iterator tit = mythis->types.begin(); tit != mythis->types.end(); tit++) {
                    if((*it)->type == tit->second) {
                        report("%-35s%11d%10s %3.1f%c%6d%2s%2s%2s\n", (*it)->name.c_str(), (*it)->priority, 
                            tit->first.c_str(), (*it)->chance*100, '%',
                            (*it)->shots,
                            (*it)->fallthrough?"F":"", (*it)->enabled?"":"D",
                            (*it)->color?"C":"");
                    }
                }
            }
        } else {
            report_err("%cgroup:'%s' is not a known group name!\n", CmdChar, opt['l'].c_str());
        }
    } else if(opt.argc() < 2) {
        report("/group: The following groups are defined:\n");
        for(hookgroups_type::iterator it = mythis->hookgroups.begin();it != mythis->hookgroups.end(); it++) {
            report("\t%s\n", it->first.c_str());
        }
    } else {
        string hookname = opt.arg(1);
        if(mythis->hooknames.find(hookname) != mythis->hooknames.end()) {
            string groups("");
            for(size_t i=0;i<mythis->hooknames[hookname]->groups.size(); i++) {
                if(groups.length()) {
                    groups += ", ";
                }
                groups += mythis->hooknames[hookname]->groups[i];
            }
            report("%cgroup: Hook %s is a member of the following groups: %s\n", CmdChar, hookname.c_str(), groups.c_str());
        } else {
            report("%cgroup: '%s' is not a known hook!\n", CmdChar, hookname.c_str());
        }
    }
    return true;
}

HookType Hook::add_type(const string& name) { 
    types[name] = (HookType)++max_type;
    hooks.push_back(new hookstubset_type());
    return (HookType)max_type; 
}

void Hook::add(HookType t, HookStub* callback) {
    HookStub* stub;
    bool priorityused = false;
    // sanity-check the new callback
    if(callback == NULL) {
        report_err("Hook::add(%d, NULL) has null callback\n", t);
        return;
    }
    callback->type = t;
    if(callback->type > max_type) {
        report_err("Hook::add(%d, %p) has HookType too big!\n", t, callback);
    }
    // HUH?
//    if(callback->name.c_str() == NULL) { callback->name = ""; }
//    if(callback->group.c_str() == NULL) { callback->group = ""; }

    if((stub = hooknames[callback->name])) {
        remove(callback->name);
        report_warn("hook %s redefined.\n", callback->name.c_str());
    }
    // Insert it into the types-indexed list
    for(hookstubset_type::iterator it = hooks[t]->begin();it != hooks[t]->end();it++) {
        if((*(*it))[0]->priority == callback->priority) { 
            (*it)->push_back(callback);
            priorityused = true;
            break; // callback can only have one priority!
        }
    }
    if(!priorityused) {
        vector<HookStub*>* newvector = new vector<HookStub*>;
        if(newvector == NULL) {
            report_err("Hook::add(%d, %p) failed to create new vector<HookStub*>\n", (int)t, callback);
            return;
        }
        newvector->push_back(callback);
        hooks[t]->insert(newvector);
    }
    callback->type = t;
    hooknames[callback->name] = callback;         // Insert it into the name-indexed list
    for(size_t i=0;i<callback->groups.size();i++) {
        hookgroups[callback->groups[i]].push_back(callback); // Insert it into the group-indexed list
    }
}

void Hook::add(const string name, HookStub* callback) {
    add(types[name], callback);
}

void Hook::addDirtCommand(string name, bool (*cbk)(string&,void*,savedmatch*), void* instance) {
    add(COMMAND, new CommandHookStub(-1, 1.0, -1, false, true, true,
        "__DIRT_COMMAND_" + name, vector<string>(1,"Dirt commands"), name, cbk, instance));
}

void Hook::run(HookType t, string& data, savedmatch* sm) {
    hookstubset_type* hookset = hooks[t];
    HookStub *stub;
    bool done = false;
    string uncolored = uncolorize(data);
    string olduncolored = uncolored;  // To check if it changes.
    string olddata = data;

    for(hookstubset_type::iterator it = hookset->begin(); it != hookset->end(); it++) {
        interpreter.mark();
        for(size_t i=0;i<(*it)->size();i++) {  // Iterate over hooks of same priority
            stub = (*(*it))[i];
            if(stub == NULL) {
                report_err("Hook::run(%d, %s) got a NULL HookStub! (i=%d)\n", t, data.c_str(), (int)i);
                done = true;
                break;
            }
            if(stub->enabled &&
              (stub->shots != 0) &&
              (stub->chance == 1.0 || (float)rand()/(float)RAND_MAX <= stub->chance)) {
                bool caught = false;
                if(stub->color) {
                    // If we have a savematch, the data of 
                    caught = stub->operator()(data, sm);  // call HookStub::operator()
                    if(data != olddata) {
                        uncolored = olduncolored = uncolorize(data);
                        olddata = data;
                    }
                } else {
                    caught = stub->operator()(uncolored, sm);        // call HookStub::operator()
                    if(uncolored != olduncolored) {
                        olddata = data = uncolored;
                        uncolored = olduncolored = uncolorize(data);
                    }
                }
                if(stub->shots > 0) stub->shots--;
                if(!stub->fallthrough && caught) {
                    done = true;
                    break; // don't process lower priority hooks
                }
            }
        }
        interpreter.execute(data);
        if(done) break; // gotta break out of both loops.
        // We should also do a select and print anything perl/python may have printed.
    }
}

void Hook::run(HookType t, char* data, savedmatch* sm) {
    string s = (data)?data:"";
    run(t, s, sm);
    if(data && data != s) strcpy(data, s.c_str());
}

// These (remove/disable/enable) are O(n) operations.  I assuming you won't be
// removing hooks in a tight loop.  If someone needs to add/remove hooks
// quickly, I should consider using a container other than vector for this
// (i.e. hash_map, hashed on name).
bool Hook::remove(string name) {
    HookStub* stub;

    // it iterates over the multiset<vector<HookStub*> >, *it and it-> are vector<>
    if((stub = hooknames[name])) {
        for(hookgroups_type::iterator git = hookgroups.begin(); git != hookgroups.end();git++) {
            for(vector<HookStub*>::iterator vit = git->second.begin();vit != git->second.end();vit++) {
                if(*vit == stub) git->second.erase(vit);
                break; // Deleting element from vector/multiset
                       // invalidates the iterator we're using.
            }
        }
        for(hookstubset_type::iterator it = hooks[stub->type]->begin();it != hooks[stub->type]->end();it++) {
            vector<HookStub*>* privec = *it;  // Vector of same priority
            if((*privec)[0]->priority == stub->priority) {
                for(vector<HookStub*>::iterator vit = privec->begin();vit != privec->end();vit++) {
                    if(*vit == stub) {
                        privec->erase(vit);
                        if(privec->empty()) {
                            hooks[stub->type]->erase(it);
                            delete privec;
                        }
                        break; // Deleting element from vector/multiset
                               // invalidates the iterator we're using.
                    }
                }
            }
        }
        hooknames.erase(hooknames.find(name));
        delete stub;
        return true;
    } else {
        report("Unable to remove hook %s, because it isn't defined.\n", name.c_str());
    }
    return false;
}

bool Hook::disable(string name) {
    HookStub* stub;

    if((stub = hooknames[name])) {
        stub->enabled = false;
        return true;
    }
    return false;
}

bool Hook::disableGroup(string group) {
    if(hookgroups.find(group) != hookgroups.end()) {
        vector<HookStub*>* v=&hookgroups[group];
        for(vector<HookStub*>::iterator it=v->begin();it !=v->end(); it++) {
            (*it)->enabled = false;
        }
        return true;
    }
    return false;
}

bool Hook::enable(string name) {
    HookStub* stub;

    if((stub = hooknames[name])) {
        stub->enabled = true;
        return true;
    }
    return false;
}

bool Hook::enableGroup(string group) {
    if(hookgroups.find(group) != hookgroups.end()) {
        vector<HookStub*>* v=&hookgroups[group];
        for(vector<HookStub*>::iterator it=v->begin();it !=v->end(); it++) {
            interpreter.add(string("/enable ") += (*it)->name);
//            (*it)->enabled = true;
        }
        return true;
    }
    return false;
}

// delete all hooks stored in hooks.
Hook::~Hook() {
    for(int i=0;i<=max_type;i++) {
        for(hookstubset_type::iterator it=hooks[i]->begin();it != hooks[i]->end(); it++) {
            delete *it;
        }
        delete hooks[i];
    }
}

CppHookStub::CppHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, 
    vector<string> g, bool (*cbk)(string&)) : HookStub(p, c, n, F, en, col, nm, g) {

    callback = cbk;
}

bool CppHookStub::operator() (string& data, savedmatch*) {
    return callback(data);
}

void CppHookStub::print() {
    report("\tCallback: 0x%p", callback);
}

CommandHookStub::CommandHookStub(int p, float c, int n, bool F, bool en, bool col,
        string nm, vector<string> g, string cmdname, bool (*cbk)(string&,void*,savedmatch*), 
        void* ins) : HookStub(p, c, n, F, en, col, nm, g) {
    commandname = cmdname;
    callback = cbk;
    instance = ins;
}

bool CommandHookStub::operator() (string& data, savedmatch* sm) {
    if(data.length() && data[0] == interpreter.getCommandCharacter() 
#if __GNUC__ >= 3
        && !data.compare(1, commandname.length(), commandname)
#else
        && !data.compare(commandname, 1, commandname.length())
#endif
        && (data.length() == commandname.length() + 1 || data[commandname.length()+1] == ' ')) {
        return(callback(data,instance,sm)); 
    }
    return false;
}

void CommandHookStub::print() {
    report("\tCommand name: %s\n", commandname.c_str());
    report("\tInstance:     %p\n", instance);
    report("\tCallback:     %p\n", callback);
}

TriggerHookStub::TriggerHookStub(int p, float c, int n, bool F, bool en, bool col, string nm, 
        vector<string> g, string rx, string arg)
        : HookStub(p, c, n, F, en, col, nm, g), regex(rx), command(arg) {
    if(regex.length() == 0) matcher = NULL;
    else { 
        matcher = embed_interp->match_prepare(regex.c_str(), arg.c_str());
        if(matcher == NULL) { // Matcher function creation failed (syntax error?) -- disable it
            en = false;
        }
    }
}

bool TriggerHookStub::operator() (string& data, savedmatch* sm) {
    bool retval;
    char out[std::max(2*(unsigned long)command.length(),1024UL)]; // FIXME buffer overflow

    //if(sm) report_err("TriggerHookStub::operator() called with existing savedmatch!");

    if(matcher) {
        retval = embed_interp->match(matcher, data.c_str(), out);   // match clobbers default_var.
        if(retval && command.length()) { 
            // SEND and COMMAND hooks must be inserted at the beginning of
            // the command stack, otherwise we will be reversing the
            // command order!  i.e. a;b -> b;a if there is a SEND hook on a.
            if(sm) {
                if(type == SEND || type == COMMAND) interpreter.insert(command, sm);
                else interpreter.add(command, sm);
            } else {
                if(type == SEND || type == COMMAND) interpreter.insert(command, data, regex);
                else interpreter.add(command, data, regex);
            }
            retval = true;
        }
    } else { 
        if(sm) {
            if(type == SEND || type == COMMAND) interpreter.insert(command, sm);
            else interpreter.add(command, sm);
        } else {
            if(type == SEND || type == COMMAND) interpreter.insert(command);
            else interpreter.add(command);
        }
        retval = true;
    }
    return retval;
}

void TriggerHookStub::print() {
    report("\tMatcher (regex): %s\n", regex.c_str());
    report("\tCommand:         %s\n", command.c_str());
}

KeypressHookStub::KeypressHookStub(int p, float c, int n, bool F, bool en, 
        bool col, string nm, vector<string> g, string regex, string arg, int k, 
        string w, bool (*cbk)(string&,void*), void* ins)
        : TriggerHookStub(p, c, n, F, en, col, nm, g, regex, arg), key(k), window(w), 
        callback(cbk), instance(ins) {
}

void KeypressHookStub::print() {
    report("\tKey:      %d\n", key);
    report("\tWindow:   %s\n", window.c_str());
    report("\tCallback: %p\n", callback);
    report("\tInstance: %p\n", instance);
}

bool KeypressHookStub::operator() (string& data, savedmatch*) {
    MessageWindow* mw;

    if(key == inputLine->getlastkey()) {
        if(window.length()) {
            if((mw = MessageWindow::find(window)) && mw->is_visible()) {
//                    report("Running KeypressHookStub for window '%s'\n", window.c_str());
                if(callback) return callback(data, instance);
                else return TriggerHookStub::operator()(data);
            } // else return false;
        } else {
//            report("Running KeypressHookStub (no window:'%s')\n", window.c_str());
            if(callback) return callback(data, instance);
            else return TriggerHookStub::operator()(data);
        }
    }
    return false;
}
