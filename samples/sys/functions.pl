
use Carp;

# Run a client command.
sub run {
    my $x = join(" ", @_);

    # Protect against accidents
#    $x =~ s/;/\\;/;
    $x =~ s/\n/ /;
    $x = $x . "\n";
#    print "@ main::run running command: $x";
    syswrite INTERP, $x, length($x);
}

# Print a message as if from the client (i.e. preceede by @)
sub report {
    $str = "@ " . join(" ", @_); 
    if($str !~ /\n$/) { $str .= "\n"; }
    print $str;
}

sub report_err {
    $str = "@ ${Red_Black}[ERROR]${White_Black} " . join(" ", @_);
    if($str !~ /\n$/) { $str .= "\n"; }
    print $str;
}

# This value is the file descriptor of the interpreter pipe and is set by
# dirt (main.cc)

open(INTERP, ">&$interpreterPipe") or die;

sub backslashify { # escape quotes
    $var = shift;
    $escapeme = shift;
    $var =~ s/\\/\\\\/g; # escape existing backslashes
    $var =~ s/$escapeme/\\$escapeme/g; # escape requested character
    return $var;
}

# is this used?
#sub main::lc {
#  &main::run(lc $_);
#}

# Create wrappers for other dirt functions
foreach (qw/load open close reopen quit speedwalk bell echo status exec window kill print alias action send help eval run setinput clear prompt send_unbuffered chat chatall/) {
    eval "sub dirt_$_ { run (\"${commandCharacter}$_ \" . join(' ', \@_)); }";
}

print "Loaded sys/functions.pl\t(Routines to access C++ dirt commands from perl)\n";

1;
