
use Carp;
use DynaLoader;

BEGIN {
    $sym_run = DynaLoader::dl_find_symbol(0,    "dirt_perl_run");
    $sym_report = DynaLoader::dl_find_symbol(0, "dirt_perl_report");
    $sym_report_err = DynaLoader::dl_find_symbol(0, "dirt_perl_report_err");
    if(defined $sym_run && defined $sym_report && defined $sym_report_err) { 
        DynaLoader::dl_install_xsub("run",    $sym_run);
        DynaLoader::dl_install_xsub("report", $sym_report);
        DynaLoader::dl_install_xsub("report_err", $sym_report_err);
    } else {
        die("The perl module for dirt was not compiled properly (symbols missing)\n");
    }
}

# This value is the file descriptor of the interpreter pipe and is set by
# dirt (main.cc)

open(INTERP, ">&$interpreterPipe") or die;

sub backslashify { # escape the second argument.
    $var = shift;
    $escapeme = shift;
    $var =~ s/\\/\\\\/g; # escape existing backslashes
    $var =~ s/$escapeme/\\$escapeme/g; # escape requested character
    return $var;
}

sub debackslashify { # unescape 
    my($var) = shift;
    my($pos) = 0;
    my($ret) = "";
    while($var =~ /\G([^\\]*?)\\(.)/g) {
        $ret .= "$1$2";
        $pos = pos($var);
    }
    $ret .= substr($var,$pos,length($var)-$pos);
    return $ret;
}

# Create wrappers for other dirt functions
foreach (qw/load open close reopen quit speedwalk bell echo status exec window kill print alias action send help eval run setinput clear prompt send_unbuffered chat chatall/) {
    eval "sub dirt_$_ { run (\"${commandCharacter}$_ \" . join(' ', \@_)); }";
}

print "Loaded sys/functions.pl\t(Routines to access C++ dirt commands from perl)\n";

1;
