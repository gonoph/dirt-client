
#include <string.h>
#include <time.h>
#include "dirt.h"
#include "Alias.h"
#include "Action.h"

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
		if (!strcasecmp(mud->name, _name))
			return mud;
	}
	
	return NULL;
}

void MUD::setHost(const char *_host, int _port) {
    hostname = _host;
    port = _port;
}

const char* MUD::getHostname() const {
    if (hostname.len() == 0)
        return inherits ? inherits->getHostname() : "";
    return hostname;
}

int MUD::getPort() const {
    if (port == 0)
        return inherits ? inherits->getPort() : 0;
    return port;
}


MUD globalMUD("global", "", 0, NULL, "");

void MUD::write(FILE *fp, bool global) {
    const char *indent;
    
    if (!global) {
        fprintf(fp, "Mud %s {\n", ~name);
        indent = "  ";
    } else
        indent = "";

    if (!global) {
        if (hostname.len())
            fprintf(fp, "  Host %s %d\n", ~hostname, port);
        if (commands.len())
            fprintf(fp, "  Commands %s\n", ~commands);
        if (inherits && inherits != &globalMUD)
            fprintf(fp, "  Inherit %s\n", ~inherits->name);
    }

    if (alias_list.size())
        fprintf(fp, "\n");
    
//    FOREACH (Alias*, alias, alias_list)
    for(hash_map<string,Alias*,hash<string> >::iterator it=alias_list.begin();it!=alias_list.end();it++)
        fprintf(fp, "%sAlias %s\t%s\n", indent, it->first.c_str(), it->second->text.c_str());

    if (action_list.count())
        fprintf(fp, "\n");
    
    FOREACH(Action*, ac, action_list) {
        if(ac->type == Action::Replacement)
            fprintf(fp, "%sSubst \"%s\"\t%s\n", indent, ~ac->pattern, ~ac->commands);
        else
            fprintf(fp, "%sAction \"%s\"\t%s\n", indent, ~ac->pattern, ~ac->commands);
    }

    if (macro_list.count())
        fprintf(fp, "\n");

    FOREACH(Macro*, macro, macro_list)
        fprintf(fp, "%sMacro %s\t%s\n", indent, key_name(macro->key), ~macro->text);

    if (!global)
        fprintf(fp, "}\n");
}

Alias* MUD::findAlias(const char *name, bool recurse) {
    if(alias_list.find(name) != alias_list.end()) return alias_list[name];
    if (inherits && recurse)
        return inherits->findAlias(name);
    else
        return NULL;
}

Macro* MUD::findMacro(int key, bool recurse) {
    FOREACH(Macro *, m, macro_list)
        if (m->key == key)
            return m;

    if (inherits && recurse)
        return inherits->findMacro(key);
    else
        return NULL;
}

void MUD::checkActionMatch(const char *s) {
    FOREACH(Action *, a, action_list)
        if (a->type == Action::Trigger)
            a->checkMatch(s);

    if (inherits)
        inherits->checkActionMatch(s);
}

void MUD::checkReplacement(char *buf, int& len, char **new_out) {
    FOREACH(Action *, a, action_list)
        if (a->type == Action::Replacement)
            a->checkReplacement(buf, len, new_out);

    if (inherits)
        inherits->checkReplacement(buf, len, new_out);
}

const char *MUD::getFullName() const {
    return Sprintf("%s@%s:%d", ~name, ~hostname, port);
}
    

