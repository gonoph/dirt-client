
#include <iostream>
#include <string>
#include "Option.h"
#include <stdlib.h>
#include <stdio.h>

string debackslashify(const string& s) {
    size_t lastpos=0,pos=0;
    string retval("");
    if((pos = s.find('\\', lastpos)) != string::npos) {
        do {
            retval.append(s.substr(lastpos, pos-lastpos));
            retval.append(s.substr(pos+1, 1));
            lastpos = pos+2;
        } while((pos = s.find('\\', lastpos)) != string::npos);
        retval.append(s.substr(lastpos, s.length()-lastpos));
        return retval;
    } else {
        return s;
    }
}

void report(const char *fmt, ...) {
    va_list va;
    int len = strlen(fmt);
    char buf[4096];
    char *newfmt = (char*)malloc(len+4);
    newfmt[0] = '@';
    newfmt[1] = ' ';
    strcpy(newfmt+2, fmt);
    if(newfmt[len+1] != '\n') {
        newfmt[len+2] = '\n';
        newfmt[len+3] = '\0';
    }
    va_start(va,fmt);	
    vsnprintf(buf, sizeof(buf)-1, newfmt,va);
    va_end(va);

//    strcat(buf, "\n");

    fprintf(stderr, "%s", buf);
            
}

void report_err(const char *fmt, ...) {
    va_list va;
    int len = strlen(fmt);
    char buf[4096];
    char *newfmt = (char*)malloc(len+25);
    strcpy(newfmt, "@ [ERROR] ");
    len = strlen(newfmt);
    strcpy(newfmt+len, fmt);
    len = strlen(newfmt);
    if(newfmt[len-1] != '\n') {
        newfmt[len] = '\n';
        newfmt[len+1] = '\0';
    }
    va_start(va,fmt);	
    vsnprintf(buf, sizeof(buf)-1, newfmt,va);
    va_end(va);

    fprintf(stderr, "%s", buf);
    free(newfmt);
}

int main() {
    OptionParser opt1("/blah -w23 -d385 -FART -t'this is my \"trigger' \"blah blah2	-A blah3\"", "AFRTCPw::d:t:");

    OptionParser opt2("/blah -w	23 -d 385 -FART   -t'this is my \\'trigger\\'' -- blah blah2	blah3 -thisisnotaflag - -- --- -m-u-t-h-a-f-u-c-k-a-", "AFRTCPw:d::t:");
    OptionParser opt3("", "cat");
    OptionParser opt4("  /echo -n -W \"myWindow\" you suck 'dipshit blah \"bunk\"'", "W:n");
    OptionParser opt5("/opt5 -W -b", "");   // Should produce error messages
    cout << endl;
    OptionParser opt6("/opt6 -b", "b:");
    cout << endl;
    OptionParser opt7("/hook -T SEND -C westforest-road westforest-road = westforest-road;road-ice", "aFilfDt:p:c:n:g:T:L:C:r:N:d:k:W:");
    cout << endl;

    OptionParser opt8("/opt8 -r \"asdf 1\"", "r"); // should generate the same restStr.
    OptionParser opt9("/opt9 -r command = /eval -Lperl @arg = split(/\\w+/,$_; print join(',',@arg);", "r");
    OptionParser opt10("/hook -T INIT -fL perl perl_init = init", "aFilfDt:p:c:n:g:T:L:C:r:N:d:k:W:");

    cout << "opt1: " << opt1.str() << endl;
    if(opt1.valid()) {
        if(opt1.gotOpt('w')) cout << "opt1 argument w: " << opt1['w'] << endl;
        if(opt1.gotOpt('d')) cout << "opt1 argument d: " << opt1['d'] << endl;
        if(opt1.gotOpt('F')) cout << "opt1 argument F: " << opt1['F'] << endl;
        if(opt1.gotOpt('A')) cout << "opt1 argument A: " << opt1['A'] << endl;
        if(opt1.gotOpt('R')) cout << "opt1 argument R: " << opt1['R'] << endl;
        if(opt1.gotOpt('T')) cout << "opt1 argument T: " << opt1['T'] << endl;
        if(opt1.gotOpt('C')) cout << "opt1 argument C: " << opt1['C'] << endl;
        if(opt1.gotOpt('P')) cout << "opt1 argument P: " << opt1['P'] << endl;
        if(opt1.gotOpt('t')) cout << "opt1 argument t: " << opt1['t'] << endl;
        if(opt1.flag('C')) { cout << "opt1 found argument C" << endl; }
        if(opt1.flag('T')) { cout << "opt1 found argument T" << endl; }
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt1.argc();i++) { cout << "\t" << opt1.arg(i) << endl; }
        cout << "restStr: " << opt1.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt1 is not valid." << endl;
    }


    cout << "opt2: " << opt2.str() << endl;
    if(opt2.valid()) {
        if(opt2.gotOpt('w')) cout << "opt2 argument w: " << opt2['w'] << endl;
        if(opt2.gotOpt('d')) cout << "opt2 argument d: " << opt2['d'] << endl;
        if(opt2.gotOpt('F')) cout << "opt2 argument F: " << opt2['F'] << endl;
        if(opt2.gotOpt('A')) cout << "opt2 argument A: " << opt2['A'] << endl;
        if(opt2.gotOpt('R')) cout << "opt2 argument R: " << opt2['R'] << endl;
        if(opt2.gotOpt('T')) cout << "opt2 argument T: " << opt2['T'] << endl;
        if(opt2.gotOpt('C')) cout << "opt2 argument C: " << opt2['C'] << endl;
        if(opt2.gotOpt('P')) cout << "opt2 argument P: " << opt2['P'] << endl;
        if(opt2.gotOpt('t')) cout << "opt2 argument t: " << opt2['t'] << endl;
        if(opt2.flag('C')) { cout << "opt2 found argument C" << endl; }
        if(opt2.flag('T')) { cout << "opt2 found argument T" << endl; }
        cout << "Other arguments(" << opt2.argc() << "): " << endl;
        for(int i=0;i<opt2.argc();i++) { cout << "\t" << opt2.arg(i) << endl; }
        cout << "restStr: " << opt2.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt2 is not valid." << endl;
    }

    cout << "opt3: " << opt3.str() << endl;
    if(opt3.valid()) {
        cout << "opt3 argument c: " << opt3['c'] << endl;
        cout << "opt3 argument a: " << opt3['a'] << endl;
        cout << "opt3 argument t: " << opt3['t'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt3.argc();i++) { cout << "\t" << opt3.arg(i) << endl; }
        cout << "restStr: " << opt3.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt3 is not valid." << endl;
    }

    if(opt4.valid()) {
        cout << "opt4: " << opt4.str() << endl;
        cout << "opt4 argument n: " << opt4['n'] << endl;
        cout << "opt4 argument W: " << opt4['W'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt4.argc();i++) { cout << "\t" << opt4.arg(i) << endl; }
        cout << "restStr: " << opt4.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt4 is not valid." << endl;
    }

    if(opt7.valid()) {
        cout << "opt7: " << opt7.str() << endl;
        cout << "opt7 argument r: " << opt7['r'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt7.argc();i++) { cout << "\t" << opt7.arg(i) << endl; }
        cout << "restStr: " << opt7.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt7 is not valid." << endl;
    }

    if(opt8.valid()) {
        cout << "opt8: " << opt8.str() << endl;
        cout << "opt8 argument r: " << opt8['r'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt8.argc();i++) { cout << "\t" << opt8.arg(i) << endl; }
        cout << "restStr: " << opt8.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt8 is not valid." << endl;
    }

    if(opt9.valid()) {
        cout << "opt9: " << opt9.str() << endl;
        cout << "opt9 argument r: " << opt9['r'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt9.argc();i++) { cout << "\t" << opt9.arg(i) << endl; }
        cout << "restStr: " << opt9.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt9 is not valid." << endl;
    }

    if(opt10.valid()) {
        cout << "opt10: " << opt10.str() << endl;
        cout << "opt10 argument r: " << opt10['r'] << endl;
        cout << "Other arguments: " << endl;
        for(int i=0;i<opt10.argc();i++) { cout << "\t" << opt10.arg(i) << endl; }
        cout << "restStr: " << opt10.restStr() << endl;
        cout << endl;
    } else {
        cout << "opt10 is not valid." << endl;
    }

}
