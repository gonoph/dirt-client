
#include "dirt.h"
#include "stdlib.h"
#include "Option.h"

//#ifndef report
//#define report printf
//#endif

// OptionParser works like getopt.  That is, the second argument to the 
// constructor specifies the valid one-character options.  A character
// followed by a colon indicates that it has a required argument.  A 
// character followed by two colons indicates that it has an optional 
// argument.  The special string '--' terminates option processing.  
// Example:
//      OptionParser opt("/window -w25 -h15 -x0 -y0 -b Joe", "w:h:x:y:bfW::")
// indicates the options -w,-h,-x,-y must have arguments (specified either 
// like "-w25" or "-w 25"), -b,-f are "boolean", and W can take an optional
// argument.

// Find -- to terminate options.

// Alternatively:
// while(s =~ /\s*(?:-([A-Za-z]))?([\'\"])?(\w+)\2/) {
//   if($1) { $options{$1} = $3?$3:1; }
// }

// Ok, this turned out much uglier than I anticipated.
OptionParser::OptionParser(const string& _s, const string& _options)
    :  s(_s), optionStr(_options), isvalid(true)
{
    unsigned int start = 0;
    unsigned int pos = 0;
    unsigned int endoptpos = 0;   // last option
    unsigned int argstart = 0;
    int numbs = 0;
    bool newarg = true;     // looking for a new argument
    bool inq = false;       // in single quotes (')
    bool inqq = false;      // in double quotes (")
    bool lookforopt = true; // haven't found a -- yet.
    bool needarg = false;   // need mandatory argument
    bool needoptarg = false;// need optional argument
    bool isoptchar = false; // this character is an option char
    bool lastwasopt = false;// was the last agument an option?
    char optchar = '\0';
    while(pos < s.length()) {
        char thisc = _s[pos];
        if(thisc == '\\') { // FIXME what if the string starts with \?
            numbs++;
        } else {
            if(thisc == '\n' || thisc == '\r' || thisc == '\t' || thisc == ' ') {
                if(!(inq || inqq || numbs%2 == 1 || newarg)) { 
                    if(needarg || needoptarg) {
                        options[optchar] = debackslashify(s.substr(start,pos-start));
                        lastwasopt = true;
                        needarg = needoptarg = false;
                        newarg = true; 
                    }
                    else if(!(isoptchar || newarg)) {
                        nonOptions.push_back(debackslashify(s.substr(start,pos-start)));
                        lastwasopt = false;
                        newarg = true;
                    }
                    if(isoptchar) { newarg = true; isoptchar = false; lastwasopt = true; }
                }
            } else {
                if(newarg) { 
                    newarg = false; 
                    start = pos;
                    if(lastwasopt) endoptpos = start;
                    if(!argstart && pos) argstart = pos;  // grab the first argument
                }
                if(thisc == '\'') {
                    if(numbs%2 == 0 && !inqq) {
                        if(needarg || needoptarg) { 
                            if(inq) {
                                options[optchar] = debackslashify(s.substr(start+1, pos-start-1));  // strip ""
                                lastwasopt = true;
                                needarg = needoptarg = false;
                                newarg = true;
                            } else {
                                start = pos;
                            }
                        } else if(inq) {
                            nonOptions.push_back(debackslashify(s.substr(start+1,pos-start-1)));
                            lastwasopt = false;
                            newarg = true;
                        }
                        inq = !inq; 
                    }
                    isoptchar = false;
                } else if(thisc == '\"') {
                    if(numbs%2 == 0 && !inq) {
                        if(needarg || needoptarg) { 
                            if(inqq) {
                                options[optchar] = debackslashify(s.substr(start+1, pos-start-1));  // strip ''
                                lastwasopt = true;
                                needarg = needoptarg = false;
                                newarg = true;
                            } else {
                                start = pos;
                            }
                        } else if(inqq) {
                            nonOptions.push_back(debackslashify(s.substr(start+1,pos-start-1)));
                            lastwasopt = false;
                            newarg = true;
                        }
                        inqq = !inqq; 
                    }
                    isoptchar = false;
                } else if(thisc == '-') {
                    if(!(inq || inqq || numbs%2 == 1) && lookforopt) {
                        if(isoptchar) {
                            lookforopt = false; // found --
                            isoptchar = false;
                            newarg = true;
                        } else if(!needarg) {  // - can be part of option.  i.e. -x -41
                            isoptchar = true; 
                            newarg = false;
                        } // next char is option char.
                    }
                } else { // thisc is a regular 'ol character.
                    if(newarg) {
                        newarg = false;
                        start = pos;
                    }
                    if(!(inq || inqq) && isoptchar) {
                        optchar = thisc;
                        size_t optloc = optionStr.find_first_of(optchar);
                        if(optloc == string::npos) {
                            report_err("Unknown option found for command %s -%c\n", nonOptions[0].c_str(), optchar);
                            isvalid = false;
                        } else if(optionStr[optloc+1] == ':') {
                            if(optionStr[optloc+2] == ':') {
                                needoptarg = true;
                                isoptchar = false;
                            } else {
                                needarg = true;
                                isoptchar = false;
                            }
                            newarg = true;
                        } else {
                            options[optchar] = "1";
                        }
                    }
                }
            }
            numbs = 0;
        }
        pos++;
    } // Scan to next argument
    if(inq || inqq) { 
        report_err("%s Syntax Error: unmatched %c.\n", nonOptions[0].c_str(), inq?'\'':'"'); 
        isvalid = false; 
    }
    if(s.length() && !(inq || inqq || numbs)) { 
        if(!(inq || inqq || numbs%2 == 1 || newarg)) { 
            if(needarg || needoptarg) {
                options[optchar] = debackslashify(s.substr(start,pos-start));
                lastwasopt = true;
                needarg = needoptarg = false;
                newarg = true; 
            }
            else if(!(isoptchar || newarg)) {
                nonOptions.push_back(debackslashify(s.substr(start,pos-start)));
                lastwasopt = false;
                newarg = true;
            }
            if(isoptchar) { newarg = true; isoptchar = false; }
        }
    }
    numbs = 0;
    if(needarg) { 
        report_err("%s: Mandatory value for option -%c not found\n", nonOptions[0].c_str(), optchar); 
        isvalid = false;
    }
    if(!endoptpos) endoptpos = argstart;
    s_nonopt = s.substr(endoptpos, s.length()-endoptpos);
    if(!isvalid) {
        report_err("Command with errors was:\n\t%s", s.c_str());
    }
}

// Returns the non-arguments part of the string, EXCLUDING argv[0]
string OptionParser::restStr() 
{
    // nonOptions[0] is the /command.
    if(nonOptions.size() == 2) return nonOptions[1];
    if(nonOptions.size() == 1) return "";
    else return s_nonopt;
}

// collapse escaped things.

string OptionParser::arg(unsigned int i)
{ 
    if(i < nonOptions.size()) return nonOptions[i]; 
    else return "";
}
