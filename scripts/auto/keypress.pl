# Keypress Package

package Keypress;
use vars qw(%Keypresses); # vars that will be exported in the Keypress:: namespace
use Getopt::Std;
if(!defined %Keypresses) {
    %Keypresses = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters
    
&main::run("/hook -T INIT -F definekeypresses = /run -Lperl Keypress::definekeypresses");
sub definekeypresses {
    foreach my $name (keys %Keypresses) {
        my($hookcmd) = "/hook -T KEYPRESS";
        foreach my $opt (keys %{$Keypresses{$name}}) {
            if($Keypresses{$name}->{$opt} && length($opt) == 1) {
                if($opt =~ /[iFeDaf]/) { 
                    $hookcmd .= " -" . $opt;
                } else {
                    $hookcmd .= " -" . $opt . " '" . &main::backslashify($Keypresses{$name}->{$opt}, '\'') . "'";
                }
            }
        }
        $hookcmd .= " -k '$name' '__DIRT_KEYPRESS_" . $name . "' = " . $Keypresses{$name}->{'action'};
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definekeypresses"); # delete myself from INIT list.
}
&main::save("Keypress::Keypresses", \%Keypresses);

&main::run("/hook -T COMMAND -C keypress keypress = /run -Lperl Keypress::command_keypress");
&main::run("/hook -T COMMAND -C key      key      = /run -Lperl Keypress::command_keypress"); # abbreviation
sub command_keypress {
    my(%opts);
    my(%keypresshash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}key(?:press)?(.*)/) { $_ = $1; }
    else { report_err("This doesn't seem to be a /keypress command!\n"); }
    while(/\G\s+(?:(-[A-Za-z]+)?\s*([\"'])(.*?[^\\](?:\\\\)*|)\2|(=)|(-[A-Za-z]*t) *($regexdelim)(.*?[^\\](?:\\\\)*)?\6|([^ \t\n"']+))/g) {
        if(defined $1) { 
            push @ARGV, $1; 
            push @ARGV, &main::debackslashify($3);
        } elsif(defined $3) { 
            push @ARGV, &main::debackslashify($3);
        } elsif(defined $4) { # The rest is the thing to be executed 
            push @ARGV, $4;
            if(m/\G\s+(.*)$/g) { push @ARGV, $1; }
            last;
        } elsif(defined $5) {
            push @ARGV, $5;
            push @ARGV, (defined $7)?$7:"";     # NO debackslashify.
        } elsif(defined $8) {
            push @ARGV, $8;
        } else {
            &main::report_err($main::commandCharacter . "keypress: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "keypress: did not reach end of argument string. \n");
    }
    getopts('d:DFg:lp:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T KEYPRESS ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-10s%-35s%11s%6s %s\n", "Key", "Action", "Priority", "Flags", "Groups"));
        foreach $name (sort keys %Keypresses) {
            my($keypressref) = $Keypresses{$name};
            &main::report(sprintf("%-10s%-35s%11s%2s%2s %s\n", $name, $keypressref->{'action'},
	    $keypressref->{p}, $keypressref->{F}?"F":"", $keypressref->{D}?"D":"",
	    $keypressref->{g}));
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Keypresses{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d '__DIRT_KEYPRESS_" . $opts{d} . "'");
            delete $Keypresses{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "keypress: cannot delete keypress '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{D}) { $hookcmd .= "-D "; $keypresshash{D} = 1; }
    else { $keypresshash{D} = ""; }
    if(defined $opts{F}) { $hookcmd .= "-F "; $keypresshash{F} = 1; }
    else { $keypresshash{F} = 0; }
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $keypresshash{g} = $opts{g}; }
    else { $keypresshash{g} = ""; }
    if(defined $opts{p}) { $hookcmd .= "-p '$opts{p}' "; $keypresshash{p} = $opts{p}; }
    else { $keypresshash{p} = 0; } # default priority

    if($#ARGV < 2) { # /keypress was dropped already.
        &main::report_err($main::commandCharacter . "keypress: Not enough arguments.  See /help keypress.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
    $keypresshash{'action'} = join(" ", @ARGV[2..$#ARGV]);
    $hookcmd .= "-k $name '__DIRT_KEYPRESS_" . $name . "' = " . $keypresshash{'action'};
    &main::run($hookcmd);
    $Keypresses{$name} = \%keypresshash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Keypresses hash accurate
&main::run("/hook -T COMMAND -C enable Keypress::command_enable = /run -Lperl Keypress::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-K/g);
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Keypresses{$2}) {
            $Keypresses{$2}->{'D'} = 0;
	    &main::run("/enable '__DIRT_KEYPRESS_$2'");
        }
        # else ignore it...it may be a hook, not a keypress.
    }
    return 1;
}

&main::run("/hook -T COMMAND -C disable Keypress::command_disable = /run -Lperl Keypress::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-K/g);
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Keypresses{$2}) {
            $Keypresses{$2}->{'D'} = 1;
	    &main::run("/disable '__DIRT_KEYPRESS_$2'");
        }
        # else ignore it...it may be a hook, not a keypress.
    }
    return 1;
}

print "Loaded auto/keypress.pl\t(Execute commands with the press of a key)\n";

1;

