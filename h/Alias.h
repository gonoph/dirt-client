#ifndef __ALIAS_H
#define __ALIAS_H

#define MAX_ALIAS_BUFFER 1024

struct Alias {
	string name;
	string text;
	
	Alias(const char *_name, const char *_text);
	void expand (const char *arg, char buf[MAX_ALIAS_BUFFER]) const;
	
	private:
	const char * find_token (const char *arg, int count) const ;
	const char * print_tokens (const char *arg, int begin, int end) const;
	
	
};

struct Macro {
	int key;
	String text;
	
	Macro(int _key, const char *_text) { key = _key; text = strdup(_text); }
};

#endif
