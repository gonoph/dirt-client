#include <EXTERN.h>
#include "dirt.h"
#include "misc.h"
#include "PerlEmbeddedInterpreter.h"
#include "Pipe.h"
#include "Interpreter.h"

#include <perl.h>
#include <unistd.h>

// Exported functions
extern "C" EmbeddedInterpreter *createInterpreter() {
    return new PerlEmbeddedInterpreter();
}

extern "C" const char* initFunction(const char *) {
    return NULL;
}

extern "C" const char* versionFunction() {
    return "Perl embedded interprter";
}

/* We have to init DynaLoader */

extern "C" {
   void boot_DynaLoader (CV* cv);
}


static void xs_init(void)
{
   newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
}

// Initialize the Perl Interpreter
PerlEmbeddedInterpreter::PerlEmbeddedInterpreter() 
    : myname("perl") {
    perl_interp = perl_alloc();
    perl_construct((PerlInterpreter*)perl_interp);
    
    char *args[] = {"dirtInternalPerlInterpreter", "-w", "-e", ""};
    if ((perl_parse(perl_interp, xs_init, 4, args, __environ)))
        error ("perl_parse error - exiting");


    // Define some bare-minimum perl stuff to get the puppy running.
    dSP;
    PUSHMARK(SP);
    // This is a little routine that works like require() but does not die if it fails.
    perl_eval_pv(
        "sub include {"
        "    my($fname) = shift;"
        "    if(defined $INC{$fname}) { delete $INC{$fname}; }"
        "    if(-f $fname) { "
        "        do $fname; "
        "        return if(defined($@) && $@); # SIG{__DIE__} should have already printed a message from the previous line"
        "        $INC{$fname} = $fname;"
        "    }"
        "    else { warn \"Could not load $fname because it does not exist.\n \"; }"
        "}" , FALSE);

    // Make all warnings pretty and easily distinguishable
    perl_eval_pv(
        "$SIG{__WARN__} = sub { "
        "    print(join \"\", map { \"\\@ \\xEA\\x04[Perl WARNING]\\xEA\\x07 $_\\n\" } split(/\\n/, join(\"\", @_)));"
        "};", FALSE);

    // Ditto for errors.
    perl_eval_pv(
        "$SIG{__DIE__} = sub { "
        "    print(join \"\", map { \"\\@ \\xEA\\x04[Perl ERROR]\\xEA\\x07 $_\\n\" } split(/\\n/, join(\"\", @_)));"
        "};", FALSE);
    if(SvOK(ERRSV) && SvTRUE(ERRSV)) {  // shouldn't ever get here.
        report_err("Error evaluating include!\n");
        report_err("\t%s\n", SvPV(ERRSV, PL_na));
    }

    default_var = perl_get_sv("_", TRUE);
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
    perl_eval_pv(s, FALSE);
        
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
    if (!(cv = perl_get_cv((char*) function, FALSE))) {
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
        char *out, savedmatch* sm, bool& haserror) {
    string oldarg(arg); // save it
    char buf[MAX_MUD_BUF]; // FIXME
    bool retval;
    int nret;
    strcpy(buf, function);
    
//    report("PerlEmbeddedInterpreter::run(%s, %s, out)", function, arg);
    if(haserror) {
        CV* cv = perl_get_cv((char*)buf,FALSE); // Just check if it exists.
        if (!cv) { 
            haserror = true;
            if(out) strcpy(out, oldarg.c_str());
            return false;
        }
    }
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
// parameters here...
    PUTBACK;
    if(sm) {
        STRLEN n_a;
        //report("PerlEmbeddedInterpreter::run: trying to rematch '%s' for function %s with matcher '%s'", sm->data.c_str(), function, SvPV((SV*)sm->matcher, n_a));
        sv_setpv(default_var, sm->data.c_str());
        nret = perl_call_sv((SV*)sm->matcher, G_EVAL|G_SCALAR|G_NOARGS);
        SPAGAIN;
        if(!nret) report_err("Matcher function did not return a value.");
        else if(!POPi) report_err("PerlEmbeddedInterpreter::run: savedmatch failed to match the second time");
    }
    if(arg != NULL) sv_setpv(default_var, arg);
    nret = perl_call_pv((char*)buf, G_EVAL|G_NOARGS|G_SCALAR|G_KEEPERR);
    SPAGAIN;
    if (SvTRUE(ERRSV)) {
        // Perl handles printing of errors for us...
        if(out) strcpy(out, oldarg.c_str()); // return unmodified argument.
        retval = true;  // What to return in this case...
        if(haserror) haserror = true;
    } else {
        if(haserror) haserror = false;
        if (out) {
            if(SvOK(default_var)) { // Prevents uninitialized value warnings of $_ = undef;
                char *s = SvPV(default_var, PL_na);
                strcpy(out,s);
            } else *out = '\0';
        }
        SV* mysv;
        if(nret == 1) {
            mysv = POPs;
            retval = SvTRUE(mysv);
        }
        else {
            retval = false;
            report("WARNING: perl function '%s' returned the wrong number of arguments.\n", function);
            if(haserror) haserror = true;
        }
    }
    PUTBACK;
    FREETMPS;
    LEAVE;
    return retval;
}

// Find pattern in str. then take commands and evaluate them, interpoloating variables
// returns a pointer to an anonym subroutine that does this, does not do
// anything itself!
void* PerlEmbeddedInterpreter::match_prepare(const char *pattern, const char *commands) {
    const char* autofn = "sub { if (/%s/) { $_ = \"%s\"; return 1; } else { $_ = \"\"; return 0; } };";
//    const char* autofn = "sub { if (/%s/) { if(defined $1) { print(\"---> \\$1: \\n\\t$1\\n\"); } $_ = sprintf(\"%%s\", \"%s\"); print \"\t$_\"; } else { $_ = \"\";} };";
    char* buf = (char*)malloc(strlen(pattern) + strlen(commands) + strlen(autofn));
    sprintf(buf, autofn, backslashify(pattern, '/').c_str(), backslashify(commands, '"').c_str());
    dSP;
    PUSHMARK(SP);
    SV* result = perl_eval_pv(buf, FALSE);
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
    count = perl_call_sv((SV*)perlsub, G_EVAL|G_SCALAR|G_NOARGS);
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
        } else
            retval = true;
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
    SV* result = perl_eval_pv(buf, FALSE);
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
//    report("eval got: '%s'", expr);
    int nret;
    bool retval;

    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    PUTBACK;
    if(sm) {
        sv_setpv(default_var, sm->data.c_str());
        nret = perl_call_sv((SV*)sm->matcher, G_EVAL|G_SCALAR|G_NOARGS);
        if(!nret) report_err("Matcher function did not return a value.");
        else if(!POPi) report_err("PerlEmbeddedInterpreter::run: savedmatch failed to match the second time");
    }
    if(arg) sv_setpv(default_var, arg);
    SV* res = perl_eval_pv((char*)expr, FALSE);
    STRLEN len;
    
    if (SvTRUE(ERRSV)) { // Perl will provide a warning message for us. (make sure init.pl gets loaded!)
        if(out) *out = NUL;
        retval = false;
    }
    else {
        if(out) {
            if(SvOK(res)) { // make sure result is not undef (or we'll get an Uninitialized value warning)
                char *s = SvPV(res, len);
                strcpy(out,s);
                out[len] = '\0';  // Make sure string is null terminated -- sometimes perl strings aren't.
            } else *out = NUL; 
        }
        retval = true;
    }
    SvREFCNT_dec(res);
    SPAGAIN;
    PUTBACK;
    FREETMPS;
    LEAVE;
    return retval;
}

// Set a named global perl variable to this value
void PerlEmbeddedInterpreter::set(const char *var, int value) {
    SV *v = perl_get_sv((char*)var, TRUE);
    sv_setiv(v,value);
}

void PerlEmbeddedInterpreter::set(const char *var, const char* value) {
    SV *v = perl_get_sv((char*)var, TRUE);
    sv_setpv(v,value);
}

int PerlEmbeddedInterpreter::get_int(const char *name) {
    SV *v = perl_get_sv((char*)name, TRUE);
    return SvIV(v);
}
    
char *PerlEmbeddedInterpreter::get_string(const char *name)
{
  SV *v = perl_get_sv((char*)name, TRUE);

  return SvPV(v, PL_na);
}
