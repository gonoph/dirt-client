#include "dirt.h"
#include "EmbeddedInterpreter.h"
#include "NullEmbeddedInterpreter.h"
#include "Plugin.h"
#include "Hook.h"
#include "Interpreter.h"
#include <dlfcn.h>
#include <unistd.h>

bool EmbeddedInterpreter::constboolfalse = false;
// Default interpreter if we don't manage to load anything
static NullEmbeddedInterpreter nullEmbeddedInterpreter;
EmbeddedInterpreter *embed_interp = &nullEmbeddedInterpreter;
static StackedInterpreter *stacked_interp;
static int interp_count = 0; // How many interpreters total do we have

void EmbeddedInterpreter::runCallouts() {
    static int last_callout_time;
    
    if(last_callout_time != current_time) {
        last_callout_time = current_time;
        hook.run(IDLE);
    }
}

StackedInterpreter::StackedInterpreter(EmbeddedInterpreter *e1,EmbeddedInterpreter *e2) 
    : interpreters()
{
    interpreters[""] = e1;
    interpreters[e1->name()] = e1;
    interpreters[e2->name()] = e2;
}

StackedInterpreter::~StackedInterpreter() {
    for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
        if(it->first == "") {  // This interpreter is in the list twice.
            interpreters.erase(it);
            continue;
        }
        delete it->second;
        interpreters.erase(it);
    }
}
void StackedInterpreter::add(EmbeddedInterpreter *e) { 
    if(interpreters.find("") == interpreters.end()) {
        interpreters[""] = e;
    }
    interpreters[e->name()] = e; 
}

// Slight code duplication here, but can't do much about it since the functions called are different,
// unless I start messing around with some functors I guess
bool StackedInterpreter::run(const char* lang, const char *function, const char *arg, char *out, bool& haserror) {
    char buf[interpreters.size()][MAX_MUD_BUF];
    int i = 0;
    bool res = false;
    bool embhaserror = haserror;
    bool err_somewhere = false;
    
    if(lang && strlen(lang) > 0) {
        embhaserror = true; // tell the embedded interpreter that it should report errors to us.
        res = interpreters[lang]->run(lang, function, arg, buf[i++], embhaserror) || res;  // || res has no effect since res is false.
        if(haserror) err_somewhere |= embhaserror;
    } else {
        for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
            if(it->first == "") continue; // skip the default one -- it's in the list twice
            embhaserror = true; // tell the embedded interpreter that it should report errors to us.
            res = it->second->run(lang, function, arg, buf[i], embhaserror) || res;
             // If there was an error in any interpreter, err_somewhere should be true
             // haserror (as passed) indicates whether we should care if the interpreter fails.
            if(haserror) err_somewhere |= embhaserror;
            arg = buf[i++];
        }
    }
    haserror = err_somewhere;

    if (out) strcpy(out, buf[i-1]);  // minus one?!?!?!?
    return res;
}

bool StackedInterpreter::load_file(const char* filename, bool suppress) {
    bool res = false;
    for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
        if(it->first == "") continue;
        res = it->second->load_file(filename, suppress) || res;
    }
    return res;
}

bool StackedInterpreter::eval(const char* lang, const char* expr, const char* arg, char* out) {
    return interpreters[lang]->eval(lang, expr, arg, out);
}

void *StackedInterpreter::match_prepare(const char* pattern, const char* replacement) {
    return interpreters[""]->match_prepare(pattern,replacement);
}

void* StackedInterpreter::substitute_prepare(const char* pattern, const char* replacement)  {
    return interpreters[""]->match_prepare(pattern,replacement);
}

bool StackedInterpreter::match(void *perlsub, const char *str, char *&out) {
    return interpreters[""]->match(perlsub, str, out);
}

void StackedInterpreter::set(const char *var, int value) {
    for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
        if(it->first == "") continue;
        it->second->set(var, value);
    }
}

void StackedInterpreter::set(const char *var, const char* value) {
    for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
        if(it->first == "") continue;
        it->second->set(var, value);
    }
}

int StackedInterpreter::get_int(const char* name) {
    return interpreters[""]->get_int(name);
}
char* StackedInterpreter::get_string(const char*name) {
    return interpreters[""]->get_string(name);
}



bool StackedInterpreter::run_quietly(const char* lang, const char* path, const char *arg, char *out, bool suppress_error) {
    char buf[interpreters.size()][MAX_MUD_BUF];
    int i = 0;
    bool res = false;
    
    if(lang && strlen(lang) > 0) {
        res = interpreters[lang]->run_quietly(lang, path, arg, buf[i], suppress_error) || res;
        arg = buf[i++];
    } else {
        for(hash_map<string,EmbeddedInterpreter*,hash<string> >::iterator it = interpreters.begin(); it != interpreters.end(); it++) {
            if(it->first == "") continue;
            res = it->second->run_quietly(lang, path, arg, buf[i], suppress_error) || res;
            arg = buf[i++];
        }
    }

    if (out && res)
        strcpy(out, buf[i-1]);

    return res;
}


vector<Plugin*> Plugin::plugins;

Plugin::Plugin(const char *_filename, void *_handle) : filename(_filename), handle(_handle) {
}

Plugin::~Plugin() {
    dlclose(handle);
}

// Unload all the scripts
void Plugin::done() {
    for(vector<Plugin*>::iterator it = plugins.begin(); it != plugins.end(); it++)
        if ((*it)->doneFunction) {
            (*it)->doneFunction();
            delete *it;
        }
}

// Load shared object and update interpreter settings
Plugin * Plugin::loadPlugin(const char *filename, const char *args) {
    char buf[1024];
    void *handle;
    
    // Try global path or home directory
    snprintf(buf, sizeof(buf), "%s/.dirt/plugins/%s", getenv("HOME"), filename);
    if (access(buf, R_OK) < 0) {
        snprintf(buf, sizeof(buf), "%s/plugins/%s", DIRT_LOCAL_LIBRARY_PATH, filename);
        if (access(buf, R_OK) < 0) {
            snprintf(buf, sizeof(buf), "%s/plugins/%s", DIRT_LIBRARY_PATH, filename);
            if (access(buf, R_OK) < 0)
                error ("Error loading %s: not found\n"
                       "I tried looking for and failed to find:\n"
                       "  %s/.dirt/plugins/%s\n"
                       "  %s/plugins/%s\n"
                       "  %s/plugins/%s\n"
                       "If you installed the standard binary distribution, you probably\n"
                       "forgot to move the plugin files to one of the above places.\n"
                       , filename, getenv("HOME"), filename, DIRT_LOCAL_LIBRARY_PATH, filename, DIRT_LIBRARY_PATH, filename);
        }
    }
    
    if (!(handle = dlopen(buf, RTLD_LAZY|RTLD_GLOBAL)))
        error ("Error loading %s: %s\n", buf, dlerror());
    
    Plugin *plugin = new Plugin(buf, handle);
    plugins.push_back(plugin);
    
    // Call the initFunction, it returns an error message in case of error
    const char *load_result;
    
    if (!(plugin->initFunction = (InitFunction*)dlsym(handle, "initFunction")))
        error ("Error loading %s: modules does not have a initFunction\n", buf);
    
    if ((load_result = plugin->initFunction(args)))
        error ("Error loading %s: %s\n", buf, load_result);
    
    // Display some information about the module if the module so wishes
    plugin->versionFunction = (VersionFunction*)dlsym(handle, "versionFunction");
    plugin->doneFunction = (DoneFunction*)dlsym(handle, "doneFunction");
    
    // Now let's setup the hooks
    if ((plugin->createInterpreterFunction = (CreateInterpreterFunction*)dlsym(handle, "createInterpreter"))) {
        EmbeddedInterpreter *interp = plugin->createInterpreterFunction();
        if (!interp)
            error ("Module %s: error creating interpreter", buf);
        else {
            if (stacked_interp) // If we already have a stacked set of interpreters, add it there
                stacked_interp->add(interp);
            else if (interp_count == 1) {
                stacked_interp = new StackedInterpreter(embed_interp, interp);
                embed_interp = stacked_interp;
            } else // this is the first interpreter
                embed_interp = interp;
        }
        interp_count++;
    }
    
    return plugin;
}

const char* Plugin::getVersionInformation() {
    if (versionFunction)
        return versionFunction();
    else
        return NULL;
}

void Plugin::displayLoadedPlugins() {
    if (plugins.size() == 0)
        report("Modules loaded: none");
    else {
        report("Modules loaded: ");
        for(vector<Plugin*>::iterator it = plugins.begin(); it != plugins.end(); it++) {
            const char *info = (*it)->getVersionInformation();
            report("%s: %s", (*it)->filename.c_str(), info ? info : "(no version information)");
        }
    }
}

// Load all plugins
void Plugin::loadPlugins(const char *plugins) {
    char module_name[INPUT_SIZE];
    char module_args[INPUT_SIZE];
    const char *s = plugins;
    char *out;

    // module_name [args], module_name [args]...
    for (;*s;) {
        while (isspace(*s))
            s++;

        out = module_name;
        while (*s && !isspace(*s) && *s != ',')
            *out++ = *s++;
        *out = NUL;
        if (!module_name[0])
            continue;

        out = module_args;
        if (*s == ' ') { // module args follow space
            out++;
            while(*s && *s != ',')
                *out++ = *s++;
        }
        *out = NUL;
        if (*s == ',')
            s++;

        loadPlugin(Sprintf("%s.so", module_name), module_args);
    }
}


bool NullEmbeddedInterpreter::load_file(const char*, bool) {
    return false;
}

bool NullEmbeddedInterpreter::run_quietly(const char*, const char*, const char*, char*, bool) {
    return false;
}

const char *EmbeddedInterpreter::findFile(const char *filename, const char *suffix) {
    // Add suffix if not there already
    if (strlen(filename) < strlen(suffix) || strcmp(filename+strlen(filename)-strlen(suffix), suffix))
        filename = Sprintf("%s%s", filename, suffix);
    
    const char *full;
    
    // Try first compared to the .dirt directory
    if (filename[0] != '/') {
        full = Sprintf("%s/.dirt/%s", getenv("HOME"), filename);
        if (access(full, R_OK) == 0) {
            return full;
        }

        // Globally installed files
        full = Sprintf("%s/lib/dirt/%s", INSTALL_ROOT, filename);
        if (access(full, R_OK) == 0) {
            return full;
        }
    }
    
    if (access(filename, R_OK) == 0)
        return filename;
    
    return NULL;
}

// Note: we don't want to use the second argument.
// When Interpreter is constructed, embed_interp points to a NullEmbeddedInterpreter.
// Then later on embed_interp is assigned a new value pointing to a StackedInterpreter.
// If we pass embed_interp to hook.add(...) when Interpreter is constructed, it will
// never see the interpreters!
bool EmbeddedInterpreter::command_load(string& str, void*) {
    OptionParser opt(str, "");
    if(!opt.valid()) return true;
    if(!embed_interp->load_file(opt.restStr().c_str())) {
        report("%cload: Unable to load file: %s\n", CMDCHAR, opt.restStr().c_str());
    }
    return true;
}

bool EmbeddedInterpreter::command_run(string& str, void*) {
    OptionParser opt(str, "L:");
    if(!opt.valid()) return true;
    char out[1024];
//    char fun[1024];

//    cout << "\n@@ " << str << endl;;
    if(opt.argc() < 2) {
        report_err("%crun: Please pass a function name to run!\n", CMDCHAR);
        return true;
    }
//    strcpy(fun, opt.arg(1).c_str());
//    cout << "@@ function name is: " << fun << endl;
    embed_interp->run(opt.gotOpt('L')?opt['L'].c_str():NULL, opt.arg(1).c_str(), NULL, out);  // FIXME second argument is incorrect.
    str = out;
    return true;
}
    
bool EmbeddedInterpreter::command_eval(string& str, void*) {
    OptionParser opt(str, "L:rs");
    if(!opt.valid()) return true;
    char out[MAX_MUD_BUF];
    embed_interp->eval(opt.gotOpt('L')?opt['L'].c_str():NULL, opt.restStr().c_str(), NULL, out);
    if(opt.gotOpt('r')) report("%ceval result: %s\n", CMDCHAR, out);
    string strout(out);
    if(opt.gotOpt('s')) interpreter.add(strout);
    return true;
}

