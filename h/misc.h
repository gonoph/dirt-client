
#include <string>
#include "defs.h"

void error (const char *fmt, ...) __attribute__((format(printf,1,2))) __attribute__((noreturn));
unsigned hash_str( char *key);
int countChar(const char *s, int c);
int longestLine (const char *s);
const char *one_argument (const char *argument, char *buf, bool smash_case);
void report(const char *fmt, ...) __attribute__((format(printf,1,2)));;
void report_warn(const char *fmt, ...) __attribute__((format(printf,1,2)));;
void report_err(const char *fmt, ...) __attribute__((format(printf,1,2)));;
const char * versionToString(int version);
string uncolorize(string& str);
string debackslashify(const string& s);
string backslashify(const string& s, char c);

// Takes an ansi code, outputs color changes
class ColorConverter {
public:
    ColorConverter();
    int convert(const byte *s, int size);
    const char *convertString(const char *s);

    bool checkReportStatus() {
        if (fReport) {
            fReport = true;
            return true;
        } else
            return false;
    }
            
private:
	bool fBold, fReport;
	int last_fg, last_bg;
};
