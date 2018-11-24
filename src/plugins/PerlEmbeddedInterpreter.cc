#include "dirt.h"
#include "misc.h"
#include "PerlEmbeddedInterpreter.h"
#include "Pipe.h"
#include "Interpreter.h"

#include <EXTERN.h>
#include <perl.h>
#include <unistd.h>

static PerlInterpreter *my_perl = NULL;

// Exported functions
extern "C" EmbeddedInterpreter *createInterpreter() {
    return new PerlEmbeddedInterpreter();
}

extern "C" const char* initFunction(const char *) {
    return NULL;
}

extern "C" const char* versionFunction() {
    return "Perl embedded interpreter";
}

/* We have to init DynaLoader */

extern "C" {
   void boot_DynaLoader (pTHX_ CV* cv);
//   void boot_Socket     (pTHX_ CV* cv);
   I32 dirt_perl_run();
   I32 dirt_perl_report();
   I32 dirt_perl_report_err();
}

// This is called from perl as run(...)
I32 dirt_perl_run() {
    dSP;                            /* initialize stack pointer      */
    SV* arg = POPs;                 /* My argument */
    STRLEN n_a;
    interpreter.add(SvPV(arg, n_a));
    return 0;  // We return no arguments
}

// FIXME this should properly handle multiple arguments
I32 dirt_perl_report() {
    dSP;
    SV* arg = POPs;
    STRLEN n_a;
    report("%s", SvPV(arg, n_a));
    return 0;  // We return no arguments
}

I32 dirt_perl_report_err() {
    dSP;
    SV* arg = POPs;
    STRLEN n_a;
    report_err("%s", SvPV(arg, n_a));
    return 0;  // We return no arguments
}

static void xs_init(pTHX)
{
// Adding stuff here is the way to call C++ from perl (see xchat source for more examples)
// newXS("Dirt::run", dirt_perl_run, "Dirt"); or something
   newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
//   newXS("Socket::bootstrap", boot_Socket, __FILE__);
}

// Initialize the Perl Interpreter
PerlEmbeddedInterpreter::PerlEmbeddedInterpreter() 
    : myname("perl") {
    char *args[] = {"dirtInternalPerlInterpreter", "-w", "-e", "0"};
    if(!my_perl) {
        my_perl = perl_alloc();
        perl_construct(my_perl);
        PL_perl_destruct_level = 1;
        perl_interp = my_perl;
        if(perl_parse(my_perl, xs_init, 4, args, (char**)NULL))
           error ("perl_parse error - exiting");
    } else perl_interp = my_perl;  // We don't ever create more than one perl interpreter.

    // Define some bare-minimum perl stuff to get the puppy running.
    //dSP;
    //PUSHMARK(SP);

    string tmp;
    report("Running push to @INC...");
    for (auto const& sp: SCRIPT_FINDER.getPaths()) {
        tmp = "push @INC, '" + sp.asPath() + "';";
        eval_pv(tmp.c_str(), TRUE);
    }
    // This is a little routine that works like require() but does not die if it fails.
    eval_pv(
        "sub include {"
        "    my($fname) = shift;"
        "    if(defined $INC{$fname}) { delete $INC{$fname}; }"
        "    if(-f $fname) { "
        "        do $fname; "
        "        return if(defined($@) && $@);" // SIG{__DIE__} should have already
        "        $INC{$fname} = $fname;"        // printed a message from the previous line
        "    }"
        "    else { warn \"Could not load $fname because it does not exist.\n \"; }"
        "}" , FALSE);

    // Make all warnings pretty and easily distinguishable
    eval_pv(
        "$SIG{__WARN__} = sub { "
        "    print(join \"\", map { \"\\@ \\xEA\\x04[Perl WARNING]\\xEA\\x07 $_\\n\" } split(/\\n/, join(\"\", @_)));"
        "};", FALSE);

    // Ditto for errors.
    eval_pv(
        "$SIG{__DIE__} = sub { "
        "    print(join \"\", map { \"\\@ \\xEA\\x04[Perl ERROR]\\xEA\\x07 $_\\n\" } split(/\\n/, join(\"\", @_)));"
        "};", FALSE);
    if(SvOK(ERRSV) && SvTRUE(ERRSV)) {  // shouldn't ever get here.
        report_err("Error evaluating include!\n");
        report_err("\t%s\n", SvPV(ERRSV, PL_na));
    }

    default_var = get_sv("_", TRUE);
}

PerlEmbeddedInterpreter::~PerlEmbeddedInterpreter() {
    perl_destruct(perl_interp);
    perl_free(perl_interp);
}

// Load up and evaluate a file
bool PerlEmbeddedInterpreter::load_file(const char *filename, bool suppress_error) {
    char s[1024];
    struct stat stat_buf;
    const char *fullname;

    if(!(fullname = findFile(filename, ".pl")) || stat(fullname, &stat_buf)) {
        return false;
    }
    sprintf(s, "do '%s';", fullname);

    dSP;
    PUSHMARK(SP);
    eval_pv(s, FALSE);
        
    if (SvOK(ERRSV) && SvTRUE(ERRSV)) {
        if(!suppress_error)
            report_err("Error in PerlEmbeddedInterpreter::load_file:\n%s", SvPV(ERRSV, PL_na));
        return false;
    }
    return true;
}

// Run a function, but do not complain if it doesn't exist
// Give up after having tried to load it once

bool PerlEmbeddedInterpreter::run_quietly(const char* lang, const char *path, const char *arg, char *out, bool suppress_error) {
    // If sys/idle is specified, function=idle but path=sys/idle
    const char *function = strrchr(path, '/');
    if (function)
        function = function+1;
    else
        function = path;

    CV *cv;
    if (!(cv = get_cv((char*) function, FALSE))) {
        char buf[256];
        sprintf(buf, "%s.pl", path);
        
        if (!load_file(buf, suppress_error)) {
//            disable_function(function);
            return false;
        }
        
    } else
        ; // free it or something? Dunno.
        
        
    return run(lang, function, arg, out);
}

// Run a function. Set $_ = arg. If out is non-NULL, copy $_ back there
// when done. return false if parse error ocurred or if function doesn't exist
bool PerlEmbeddedInterpreter::run(const char*, const char *function, const char *arg, 
        char *out, savedmatch* sm, bool haserror) {
    string oldarg; // save it
    if(arg) oldarg = arg;
    char buf[MAX_MUD_BUF]; // FIXME
    bool retval;
    strcpy(buf, function);
    string cmd = "";
    
    if(haserror) {
        CV* cv = get_cv((char*)buf,FALSE); // Just check if it exists.
        if (!cv) { 
            haserror = true;
            if(out) strcpy(out, oldarg.c_str());
            return false;
        }
    }
    if(sm) {
        sv_setpv(default_var, sm->data.c_str());
        cmd += "/" + backslashify(sm->regex, '/') + "/; ";
    } else if(arg != NULL) sv_setpv(default_var, arg);
    cmd += "&" + string(buf) + "(); ";
    SV* res = eval_pv(cmd.c_str(), FALSE);
    if (SvTRUE(ERRSV)) { // Perl handles printing of errors for us...
        if(out) strcpy(out, oldarg.c_str()); // return unmodified argument.
//        report("PerlEmb... out: %s\n", out);
        retval = true;  // What to return in this case...
        if(haserror) haserror = true;
    } else { // No error
        if(haserror) haserror = false;
        if (out) {
            if(SvOK(default_var)) { // Prevents uninitialized value warnings of $_ = undef;
                char *s = SvPV(default_var, PL_na);
                strcpy(out,s);
            } else *out = '\0';
        }
        retval = SvTRUE(res);
    }
    //report("PerlEmbeddedInterpreter::run(%s, %s, %s)", function, arg, out);
    return retval;
}

// Find pattern in str. then take commands and evaluate them, interpoloating variables
// returns a pointer to an anonym subroutine that does this, does not do
// anything itself!
void* PerlEmbeddedInterpreter::match_prepare(const char *pattern, const char *commands) {
    const char* autofn = 
        "sub { if (/%s/) { $_ = \"%s\"; return 1; } else { $_ = \"\"; return 0; } };";
    string bcommands = backslashify(commands, '"');
    string bpattern = backslashify(pattern, '/');
    char* buf = (char*)malloc(bpattern.length() + bcommands.length() + strlen(autofn));
    sprintf(buf, autofn, bpattern.c_str(), bcommands.c_str());
    dSP;
    PUSHMARK(SP);
    SV* result = eval_pv(buf, FALSE);
    free(buf);
    if(SvTRUE(ERRSV)) {
        report_err("Unable to evaluate regular expression: %s\n", pattern);
        report_err("\t%s\n", SvPV(ERRSV, PL_na));
        return NULL;
    }
    return result;
}

// Actually execute the match. Actually is like perl_run except it takes
// a SV* as first parameter *and* it returns 0 if the match fails
bool PerlEmbeddedInterpreter::match(void *perlsub, const char *str, char *const &out) {
    bool retval;
    int count;
    sv_setpv(default_var, str);
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    PUTBACK;
    count = call_sv((SV*)perlsub, G_EVAL|G_SCALAR|G_NOARGS);
    SPAGAIN;
    if(!count) report_err("Matcher function did not return a value.");
    if (SvTRUE(ERRSV)) {
        report_err("Unable to evaluate autocreated function: %s",
               SvPV(ERRSV, PL_na));
        retval = false;
    }
    else {
        char *s = SvPV(default_var, PL_na);
        if(out) strcpy(out, s);
        if(count && !POPi) {
            if(strlen(s)) report_err("PerlEmbeddedInterpreter::match: strlen(s) thinks the match succeded but return value disagrees.");
            retval = false;
        } else {
            retval = true;
        }
    }
    PUTBACK;
    FREETMPS;
    LEAVE;
    return retval;
}

// As perl_match_prepare except for substitutes
void* PerlEmbeddedInterpreter::substitute_prepare(const char *pattern, const char *replacement) {
    const char* autofn = "sub { unless (s/%s/%s/g) { $_ = \"\";} };";
    char* buf = (char*)malloc(strlen(pattern) + strlen(replacement) + strlen(autofn));
    sprintf(buf, autofn, pattern, replacement);
    dSP;
    PUSHMARK(SP);
    SV* result = eval_pv(buf, FALSE);
    if(SvTRUE(ERRSV)) {
        report_err("Error evaluating regular expression: %s\n", pattern);
        report_err("\t%s\n", SvPV(ERRSV, PL_na));
        return NULL;
    }
    return result;
}

// Evalute some loose perl code, put the result in out if non-NULL
bool PerlEmbeddedInterpreter::eval(const char*, const char *expr, const char* arg, 
        char* out, savedmatch* sm) {
    bool retval;
    string cmd = "";

    if(sm) {
        sv_setpv(default_var, sm->data.c_str());
        cmd += "/" + backslashify(sm->regex, '/') + "/; ";
    } else if(arg != NULL) sv_setpv(default_var, arg);
    cmd += expr;
    SV* res = eval_pv(cmd.c_str(), FALSE);
    
    if (SvTRUE(ERRSV)) {      // Perl will provide a warning message for us. 
        if(out) *out = 0;   //(make sure init.pl gets loaded!)
        retval = false;
    } else {
        if(out) {
            if(SvOK(default_var)) { // Prevents uninitialized value warnings of $_ = undef;
                char *s = SvPV(default_var, PL_na);
                strcpy(out,s);
            } else *out = '\0';
            retval = SvTRUE(res);
        }
        retval = SvTRUE(res);
    }
    return retval;
}

// Set a named global perl variable to this value
void PerlEmbeddedInterpreter::set(const char *var, int value) {
    SV *v = get_sv((char*)var, TRUE);
    sv_setiv(v,value);
}

void PerlEmbeddedInterpreter::set(const char *var, const char* value) {
    SV *v = get_sv((char*)var, TRUE);
    sv_setpv(v,value);
}

int PerlEmbeddedInterpreter::get_int(const char *name) {
    SV *v = get_sv((char*)name, TRUE);
    return SvIV(v);
}
    
char *PerlEmbeddedInterpreter::get_string(const char *name)
{
  SV *v = get_sv((char*)name, TRUE);

  return SvPV(v, PL_na);
}
