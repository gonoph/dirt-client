# Trigger Package

package Trigger;
use vars qw(%Triggers); # vars that will be exported in the Trigger:: namespace
use Getopt::Std;
if(!defined %Triggers) {
    %Triggers = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters
    
&main::run("/hook -T INIT -F definetriggers = /run -Lperl Trigger::definetriggers");
sub definetriggers {
    foreach my $name (keys %Triggers) {
        my($hookcmd) = "/hook -T OUTPUT";
        foreach my $opt (keys %{$Triggers{$name}}) {
            if($Triggers{$name}->{$opt} && length($opt) == 1) {
                if($opt =~ /[iFeDaf]/) { 
                    $hookcmd .= " -" . $opt;
                } else {
                    $hookcmd .= " -" . $opt . " '" . &main::backslashify($Triggers{$name}->{$opt}, '\'') . "'";
                }
            }
        }
        $hookcmd .= " '__DIRT_TRIGGER_" . $name . "' = " . $Triggers{$name}->{'action'};
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definetriggers"); # delete myself from INIT list.
}
&main::save("Trigger::Triggers", \%Triggers);

&main::run("/hook -T COMMAND -C trigger trigger = /run -Lperl Trigger::command_trigger");
&main::run("/hook -T COMMAND -C trig    trig    = /run -Lperl Trigger::command_trigger");   # abbreviation
sub command_trigger {
    my(%opts);
    my(%trighash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}trig(?:ger)?(.*)/) { $_ = $1; }
    else { &main::report_err("This doesn't seem to be a /trig command!\n"); }
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
            &main::report_err($main::commandCharacter . "trig: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "trig: did not reach end of argument string. \n");
    }
    getopts('ac:d:DFg:ln:p:t:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T OUTPUT ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-35s%11s%7s%6s%6s %s\n", "Name", "Priority", "Chance", "Shots", "Flags", "Groups"));
        foreach $name (sort keys %Triggers) {
            my($trigref) = $Triggers{$name};
            &main::report(sprintf("%-35s%11s%7s%6s%2s%2s%2s %s\n", $name, $trigref->{p}, $trigref->{c}, 
                $trigref->{n}, $trigref->{F}?"F":"", $trigref->{D}?"D":"", $trigref->{a}?"C":"", $trigref->{g}));
            &main::report("\tTriggering on: ", $trigref->{t});
            &main::report("\tAction:        ", $trigref->{'action'});
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Triggers{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d '__DIRT_TRIGGER_" . $opts{d} . "'");
            delete $Triggers{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "trig: cannot delete trigger '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{a}) { $hookcmd .= "-a "; $trighash{a} = 1; }
    else { $trighash{a} = 0; }
    if(defined $opts{c}) { $hookcmd .= "-c '$opts{c}' "; $trighash{c} = $opts{c}; }
    else { $trighash{c} = 1.0; } # default chance
    if(defined $opts{D}) { $hookcmd .= "-D "; $trighash{D} = 1; }
    else { $trighash{D} = ""; }
    if(defined $opts{F}) { $hookcmd .= "-F "; $trighash{F} = 1; }
    else { $trighash{F} = 0; }
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $trighash{g} = $opts{g}; }
    else { $trighash{g} = ""; }
    if(defined $opts{n}) { $hookcmd .= "-n '$opts{n}' "; $trighash{n} = $opts{n}; }
    else { $trighash{n} = -1; } # default shots = infinite
    if(defined $opts{p}) { $hookcmd .= "-p '$opts{p}' "; $trighash{p} = $opts{p}; }
    else { $trighash{p} = 0; } # default priority
    if(defined $opts{t}) { $hookcmd .= "-t '" . &main::backslashify($opts{t}, "'") . "' "; $trighash{t} = $opts{t}; }
    else { $trighash{t} = ""; }

    if($#ARGV < 2) { # /trig was dropped already.
        &main::report_err($main::commandCharacter . "trig: Not enough arguments.  See /help trig.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
    $trighash{'action'} = join(" ", @ARGV[2..$#ARGV]);
    $hookcmd .= "'__DIRT_TRIGGER_" . $name . "' = " . $trighash{'action'};
    &main::run($hookcmd);
    $Triggers{$name} = \%trighash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Triggers hash accurate
&main::run("/hook -T COMMAND -C enable Trigger::command_enable = /run -Lperl Trigger::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-T/g);
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Triggers{$2}) {
            $Triggers{$2}->{'D'} = 0;
	    &main::run("/enable '__DIRT_TRIGGER_$2'");
        }
        # else ignore it...it may be a hook, not a trigger.
    }
    return 1;
}

&main::run("/hook -T COMMAND -C disable Trigger::command_disable = /run -Lperl Trigger::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-T/g);
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Triggers{$2}) {
            $Triggers{$2}->{'D'} = 1;
	    &main::run("/disable '__DIRT_TRIGGER_$2'");
        }
        # else ignore it...it may be a hook, not a trigger.
    }
    return 1;
}

print "Loaded auto/trigger.pl\t(Execute commands on mud output)\n";

1;

