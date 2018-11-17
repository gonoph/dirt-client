#include "MUD.h"
#include "StaticBuffer.h"

#include <string.h>
#include <time.h>

// Class for handling a MUD
MUD::MUD (const char *_name, const char *_hostname, int _port, MUD *_inherits, const char *_commands) {
    name = _name;
    hostname = _hostname;
    port = _port;
    commands = _commands;
    inherits = _inherits;
    loaded = false;
}

MUD * MUDList::find(const char *_name) {
    for (MUD *mud = rewind(); mud; mud = next()) {
        if (!strcasecmp(mud->getName(), _name))
            return mud;
    }

    return NULL;
}

void MUD::setHost(const char *_host, int _port) {
    hostname = _host;
    port = _port;
}

const char* MUD::getHostname() const {
    if (hostname.length() == 0)
        return inherits ? inherits->getHostname() : "";
    return hostname.c_str();
}

int MUD::getPort() const {
    if (port == 0)
        return inherits ? inherits->getPort() : 0;
    return port;
}


MUD globalMUD("global", "", 0, NULL, "");

void MUD::write(FILE *fp, bool global) {
    
    if (!global) {
        fprintf(fp, "Mud %s {\n", name.c_str());
    }

    if (!global) {
        if (hostname.length())
            fprintf(fp, "  Host %s %d\n", hostname.c_str(), port);
        if (commands.length())
            fprintf(fp, "  Commands %s\n", commands.c_str());
        if (inherits && inherits != &globalMUD)
            fprintf(fp, "  Inherit %s\n", inherits->getName());
    }

    if (!global)
        fprintf(fp, "}\n");
}

const char *MUD::getFullName() const {
    return Sprintf("%s@%s:%d", name.c_str(), hostname.c_str(), port);
}
