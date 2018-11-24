#ifndef DIRT_PLUGIN_H
#define DIRT_PLUGIN_H

class EmbeddedInterpreter;
#include <vector>

class Plugin {
public:
    typedef const char* InitFunction(const char*);
    typedef void DoneFunction();
    typedef const char* VersionFunction();
    typedef EmbeddedInterpreter * CreateInterpreterFunction();

    Plugin(const string _filename, void *_handle);
    Plugin(const char *_filename, void *_handle);
    ~Plugin();
    const char *getVersionInformation();

    string filename;

private:
    void *handle; // dlopen handle
    InitFunction *initFunction;
    VersionFunction *versionFunction;
    CreateInterpreterFunction *createInterpreterFunction;
    DoneFunction *doneFunction;

public:
    static void loadPlugins(const char*); // load a list of plugins
    static void done();                   // signal to all plugins we are leaving
    static void displayLoadedPlugins();           // on the output window

private:
    static Plugin* loadPlugin(const char *,const char*); // load a particular plugin file
    static vector<Plugin*> plugins;
};

#endif
