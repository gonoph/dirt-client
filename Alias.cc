#include <dirt.h>
#include "cui.h"
#include "Alias.h"
#include "Action.h"
#include "Interpreter.h"
#include "Session.h"

// Aliases and such.

Alias::Alias(const char *_name, const char *_text)
 : name(_name), text(_text)
{
}

// Find where the count-th token starts
const char * Alias::find_token (const char *arg, int count) const {
	int current;
	const char *pc;
	
	for (current =1, pc = arg;*pc ; current++)
	{
		while (isspace(*pc))
			pc++;		

		if (count == current)
			return pc;
		
		while (*pc && !isspace(*pc))
			pc++;

	}

	return NULL;
}

// print tokens 'begin' to 'end'
const char * Alias::print_tokens (const char *arg, int begin, int end) const
{
	static char buf[MAX_ALIAS_BUFFER];
	const char *pBegin, *pEnd;
	
	if (begin == 0)
	{
		strcpy (buf, arg);
		return arg;
	}
	
	if (!(pBegin = find_token(arg,begin)))
		return NULL;
		
	if (!(pEnd = find_token(arg,end+1)))
		pEnd = arg+strlen(arg);

	memcpy(buf,pBegin,pEnd-pBegin);	
	buf[pEnd-pBegin] = NUL;
	
	return buf;
}

void Alias::expand (const char *arg, char buf[MAX_ALIAS_BUFFER]) const
{
	const char *pc;
	char *out = buf;
	
	for (pc = text.c_str();*pc && out-buf < (MAX_ALIAS_BUFFER-4); pc++)  {
        if (*pc == '%')
		{
			int begin,end;
			bool fBegin = false, fEnd = false;
			
			if (*++pc == '-')
			{
				fBegin = true;
				pc++;
			}
			else if (*pc == '+')
			{
				fEnd = true;
				pc++;
			}
			
			if (*pc && isdigit(*pc))
			{
				begin = end = *pc - '0';
				if (fBegin)
					begin = 1;
				
				if (fEnd)
					end = MAX_ALIAS_BUFFER; // just some large value
				
				const char *tokens = print_tokens(arg,begin,end);
				if (tokens && (strlen(tokens) + out-buf < MAX_ALIAS_BUFFER-4))
					out += sprintf(out, "%s", tokens);
			}
			else if (*pc == '%')
                *out++ = *pc;

            else // leave it alone
                out+= sprintf(out, "%%%c", *pc);
			
		}
		else
			*out++ = *pc;
	}
	
	*out++ = NUL;

}

AliasSelection::AliasSelection (Window *_parent, MUD *mud, const char *title)
: Selection (_parent, _parent->width, _parent->height/2, 0, _parent->height/4+1)
{
    set_top_message(title);

    for (;mud;) {
        if (mud == &globalMUD)
            add_string(">>>> Aliases global to all sessions", bg_blue|fg_yellow|fg_bold);
        else
            add_string(Sprintf(">>>> Aliases specific for mud %s (%s %d)", ~mud->name, mud->getHostname(), mud->getPort()), bg_blue|fg_yellow|fg_bold);
        for(hash_map<string, Alias*, hash<string> >::iterator it = mud->alias_list.begin();it!=mud->alias_list.end();it++) {
//            fprintf(fp, "%sAlias %s\t%s\n", indent, it->first, it->second);
//        for(int i=0;i<mud->alias_list.size();i++)
//        FOREACH(Alias*, a, mud->alias_list)
            add_string(Sprintf("%-30s %s", it->first.c_str(), it->second->text.c_str()));
        }
        mud = mud->inherits;
    }
}

ActionSelection::ActionSelection(Window *_parent, const char *title) :
Selection(_parent, wh_full, wh_half, 0, _parent->height/4+1) {
    MUD *mud;
    if (currentSession)
        mud = &currentSession->mud;
    else
        mud = &globalMUD;
    
    for (;mud;) {
        if (mud == &globalMUD)
            add_string(">>>> Actions global to all sessions", bg_blue|fg_yellow|fg_bold);
        else
            add_string(Sprintf(">>>> Actions specific for mud %s (%s %d)", ~mud->name, mud->getHostname(), mud->getPort()), bg_blue|fg_yellow|fg_bold);
        FOREACH(Action*, a, mud->action_list)
            add_string(Sprintf("%c%-40s %s", a->type == Action::Replacement ?'*': ' ', ~a->pattern, ~a->commands));
        mud = mud->inherits;
    }

    set_top_message(title);
}


MacroSelection::MacroSelection(Window *_parent, const char *title) :
Selection(_parent, wh_full, wh_half, 0, _parent->height/4+1) {
    MUD *mud;
    if (currentSession)
        mud = &currentSession->mud;
    else
        mud = &globalMUD;
    
    for (;mud;) {
        if (mud == &globalMUD)
            add_string(">>>> Macros global to all sessions", bg_blue|fg_yellow|fg_bold);
        else
            add_string(Sprintf(">>>> Macros specific for mud %s (%s %d)", ~mud->name, mud->getHostname(), mud->getPort()), bg_blue|fg_yellow|fg_bold);
        FOREACH(Macro*, m, mud->macro_list)
            add_string(Sprintf("%-10s %s", key_name(m->key), ~m->text));
        mud = mud->inherits;
    }

    set_top_message(title);
}

void Action::checkReplacement(char *buf, int &len, char **new_out) {
    char *result;
    if (embed_interp->match(embedded_sub, buf, result)) {
        // Compare result length to previous length, adjust new_out
        // accordingly

        int new_len = (result) ? strlen(result) : 0;
        memcpy((*new_out)-len, result, new_len);
        *new_out  += new_len-len;
        len = new_len;
        strcpy(buf, result);
    }
}

// Check if this thing matches, if so, run it
void Action::checkMatch (const char *buf) {
    char *result;
    if (embed_interp->match(embedded_sub, buf, result)) {
        interpreter.add(result);
    }
}

void Action::compile() {
    if (type == Trigger)
        embedded_sub = embed_interp->match_prepare(pattern, commands);
    else if (type == Replacement)
        embedded_sub = embed_interp->substitute_prepare(pattern, commands);
}

Action::Action(const char *_pattern, const char *_commands, action_t _type) : commands(_commands), type(_type) {
    pattern = _pattern;
    commands = _commands;
    if (embed_interp) // still loading config file and don't know wether we use perl or Python
        compile();
}

Action::~Action() {}

// TODO use a regexp here
Action* Action::parse(const char *buf, String& res, action_t type) {
    char name[MAX_MUD_BUF];
    char value[MAX_MUD_BUF];
    const char *s = buf;
    const char *typeName;
    
    if (type == Trigger)
        typeName = "action";
    else
        typeName = "substitute";
    
    char *out = name;
    while(isspace(*s))
        s++;
    
    if (*s == '"') {
        s++;
        while(*s && *s != '"')
            *out++ = *s++;
        
        *out = NUL;
        
        if (!*s) {
            res.printf ("Incomplete %s: %s", typeName, buf);
            return NULL;
        }
        s++;
        
    } else {
        while(!isspace(*s))
            *out++ = *s++;
        
        *out = NUL;
    }
    
    while(isspace(*s))
        s++;
    
    if (!*s && type != Replacement) {
        res.printf ("Missing %s string for %s: %s", typeName, typeName, buf);
        return NULL;
    }
    
    out = value;
    while(*s)
        *out++ = *s++;
    *out = NUL;
    
    return new Action(name, value, type);
}
