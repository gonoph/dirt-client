# Trigger Package

#% COMMAND /trigger [list|add|add_dis|delete|enable|disable|action|help]

=head1 /trig    [list|add|add_dis|delete|enable|disable|action|help]

execute commands in response to data sent from the mud

=head1 USAGE

/def from TinyFugue:
    I<add>            Add a trigger
    I<delete>         Delete a trigger
    -n<number>        number of times to execute
    -E<expr>          evaluate expression first, if true, do trigger
    -t<regex>         trigger pattern
    -b<key>           bind to key (string)
    -B<key>           bind to named key
    -p<pri>           priority                  (use order instead?)
    -c<chance>        probability that trigger will execute
    -F                fall-through (not fall through option?)
    -a[ngGurfdBbhC]   attributes (normal/gag/norecord/underline/reverse/flash/dim/bold/bell/hilite/Color)
    -P[<n>][nurfdBhC] partial hilite (<n>th subexpression in regex
    -i                invisible
    -A                match ANSI too (ANSI normally stripped and string is straight text)
    <name>
    = <body>

Regex should be delimited as perl regex (find regex to match regex), plus extra modifier:
    m//c
c denotes match "color" rather than ignore/strip it.

=cut

package Trigger;
use vars qw(%Triggers); # vars that will be exported in the Trigger:: namespace
use Getopt::Std;
if(!defined %Triggers) {
    %Triggers = ();
} #else {
    
&main::run("/hook -T INIT -F -fL perl definetriggers = Trigger::definetriggers");
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
        $hookcmd .= " " . $name . " = " . $Triggers{$name}->{'action'};
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definetriggers"); # delete myself from INIT list.
}
&main::save("Trigger::Triggers", \%Triggers);

&main::run("/hook -T COMMAND -fL perl -C trigger trigger = Trigger::command_trigger");
&main::run("/hook -T COMMAND -fL perl -C trig trig = Trigger::command_trigger");   # abbreviation
sub command_trigger {
    my(%opts);
    my(%trighash);
    my($name);
#    my($defaultvar) = $_;  # in case someone 
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}trig(?:ger)?(.*)/) { $_ = $1; }
    else { report_err("This doesn't seem to be a /trig command!\n"); }
    while(/\G\s+(?:(-[A-Za-z]+)?\"([^\"]*?)\"|(-[A-Za-z]+)?\'([^\']*?)\'|(=)|([^ \t\n"']+)|(?:\/(.*?(?:\\\\)*)\/))/g) {
        if(defined $1) { 
            push @ARGV, $1; 
            push @ARGV, $2;
#            &main::report("pushing arg 1: ", $1);
#            &main::report("pushing arg 2: ", $2);
        } elsif(defined $2) { 
            push @ARGV, $2;
#            &main::report("pushing arg 2: ", $2);
        } elsif(defined $3) {
            push @ARGV, $3;
            push @ARGV, $4;
#            &main::report("pushing arg 3: ", $3);
#            &main::report("pushing arg 4: ", $4);
        } elsif(defined $4) {
            push @ARGV, $4;
#            &main::report("pushing arg 4: ", $4);
        } elsif(defined $5) { # The rest is the thing to be executed 
            push @ARGV, $5;
#            &main::report("pushing arg 5: ", $5);
            if(m/\G\s+(.*)$/g) {
                push @ARGV, $1;
#                &main::report("pushing rest: ", $1);
            }
            last;
        } elsif(defined $6) {
            push @ARGV, $6;
#            &main::report("pushing arg 6: ", $6);
        } elsif(defined $7) {
            push @ARGV, $7;
#            &main::report("pushing arg 7: ", $7);
        } else {
            &main::report_err($main::commandCharacter . "trig: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "trig: did not reach end of argument string. \n");
    }
#    print("\@ARGV is: ", join(",", @ARGV) . "\n");
#    @ARGV = split /\s+/, $_;  # FIXME we need to split such that we respect quotes!
#    shift(@ARGV);  # drop /trig
#    Getopt::Long::Configure("no_ignore_case");
#    $Getopt::Long::bundling=1;
#    GetOptions(\%opts, "l:s", "D+", "F+", "a+", "f+", "d=s", "p=i", "c=f", "n=i", "g=s", "L=s", "t=s");
    getopts('lDFafd:p:c:n:g:L:t:', \%opts);
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
            &main::run($main::commandCharacter . "hook -d " . $opts{d});
            delete $Triggers{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "trig: cannot delete trigger '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{F}) { $hookcmd .= "-F "; $trighash{F} = 1; }
    else { $trighash{F} = 0; }
    if(defined $opts{a}) { $hookcmd .= "-a "; $trighash{a} = 1; }
    else { $trighash{a} = 0; }
    if(defined $opts{D}) { $hookcmd .= "-D "; $trighash{D} = 1; }
    else { $trighash{D} = ""; }
    if(defined $opts{f}) { $hookcmd .= "-f "; $trighash{f} = 1; }
    else { $trighash{f} = 0; }
    if(defined $opts{p}) { $hookcmd .= "-p '$opts{p}' "; $trighash{p} = $opts{p}; }
    else { $trighash{p} = 0; } # default priority
    if(defined $opts{c}) { $hookcmd .= "-c '$opts{c}' "; $trighash{c} = $opts{c}; }
    else { $trighash{c} = 1.0; } # default chance
    if(defined $opts{n}) { $hookcmd .= "-n '$opts{n}' "; $trighash{n} = $opts{n}; }
    else { $trighash{n} = -1; } # default shots = infinite
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $trighash{g} = $opts{g}; }
    else { $trighash{g} = ""; }
    if(defined $opts{L}) { $hookcmd .= "-L '$opts{L}' "; $trighash{L} = $opts{L}; }
    else { $trighash{L} = ""; }
    if(defined $opts{t}) { $hookcmd .= "-t '$opts{t}' "; $trighash{t} = $opts{t}; }
    else { $trighash{t} = ""; }

    if($#ARGV < 2) { # /trig was dropped already.
        &main::report_err($main::commandCharacter . "trig: Not enough arguments.  See /help trig.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
#    &main::report("name is: $name");
    $trighash{'action'} = join(" ", @ARGV[2..$#ARGV]);
    $hookcmd .= "'" . $name . "' = " . $trighash{'action'};
#    &main::report("hook command is: $hookcmd");
    &main::run($hookcmd);
    $Triggers{$name} = \%trighash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Triggers hash accurate
&main::run("/hook -T COMMAND -FfL perl -C enable Trigger::command_enable = Trigger::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+([A-Za-z_:]+)/g) {
        if(defined $Triggers{$1}) {
            $Triggers{$1}->{'D'} = 0;
        }
        # else ignore it...it may be a hook, not a trigger.
    }
    return 1;
}

&main::run("/hook -T COMMAND -FfL perl -C disable Trigger::command_disable = Trigger::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+([A-Za-z_:]+)/g) {
        if(defined $Triggers{$1}) {
            $Triggers{$1}->{'D'} = 1;
        }
        # else ignore it...it may be a hook, not a trigger.
    }
    return 1;
}




# Apply triggers to mud output
sub trigscan {
    return unless defined $_;
#    print "@ Trigscan: '$_'\n";:
    $colored = $_;
    $uncolored = $_;
    $uncolored =~ s/\xEA.//sg;
#    $_ = $uncolored;
    my ($t, $a);
    foreach $t (keys %Triggers) {
#        print "@   Trigscan trying $t\n";
        next unless $uncolored =~ /$t/ && $Triggers{$t}->[1];
        $a = $Triggers{$t}->[0];
#        print "@ Running command: '$a'\n";
        eval "\&main::run(\"$a\");";
#        if($a =~ /\$/ && $a !~ /^$commandCharacter/) {
#            eval "\&main::dirt_send(\"$a\");";
#        } else {
#            my($default_var) = $_;
#            $_ = $a;
#            &main::hook_run("send");
#            $_ = $default_var;
#        }
	last;
    }
}


# command '/trigger' (list/delete/enable/disable/action/add/help/add_dis)
sub old_trigger {
    my ($cmd, $trigstr, $trigact);
    
    ($cmd, $_) = split /\s+/, $_, 2;
    $cmd = "" unless defined $cmd;
    $_ = "" unless defined $_;
    if ($cmd eq "help") {
	&trigger_help();
    } elsif ($cmd eq "" || $cmd eq "list") {
	if ($cmd eq "") {
	    print "For help, type: trigger help\n";
	}
	&trigger_list();
    } elsif ($cmd eq "delete" || $cmd eq "enable" || $cmd eq "disable"
	     || $cmd eq "action") {
	my $i = 0;
	unless (/^(\d+).*/) {
	    print "trigger: $cmd: invalid argument\n";
	    return;
	}
	foreach $t (keys %Triggers) {
	    next unless ++$i == $1;
	    if ($cmd eq "delete") {
		print "trigger: deleting trigger $i\n";
		delete $Triggers{$t};
	    } elsif ($cmd eq "enable") {
		print "trigger: enabling trigger $i\n";
		$Triggers{$t}->[1] = 1;
	    } elsif ($cmd eq "disable") {
		print "trigger: disabling trigger $i\n";
		$Triggers{$t}->[1] = 0;
	    } elsif ($cmd eq "action") {
		if (/^(\d+)\s+(.*)/) {
		    $Triggers{$t}->[0] = $2;
		    print "trigger: redefined action of trigger $i to: $2\n";
		} else {
		    print "trigger: action: invalid syntax\n";
		}
	    }
	    return;
	}
	print "trigger: $cmd: $1: no such trigger\n";
    } elsif ($cmd eq "add" || $cmd eq "add_dis") {
	if (/^("([^\\"]*(\\.|"))*)\s+(.*)/) {  # huh?
	    $trigstr = $1;
	    $trigact = $4;
	    $trigstr =~ s/.//;
	    $trigstr =~ s/.$//;
	    $trigstr =~ s/\\"/"/g;
        } elsif (/^(\/([^\\\/]*(\\.|\/))*)\s+(.*)/) {
            $trigstr = $1;
            $trigact = $4;
	    $trigstr =~ s/.//;
	    $trigstr =~ s/.$//;
	    $trigstr =~ s/\\\//\//g;
	} elsif (/^(\S+)\s+(.*)/) {
	    $trigstr = $1;
	    $trigact = $2;
	} else {
	    print "trigger: add: invalid syntax\n";
	    return;
	}
	unless ($trigact ne "") {
	    print "trigger: add: empty action string\n";
	    return;
	}
	$Triggers{$trigstr} = [ $trigact, 1 ];
	$Triggers{$trigstr}->[1] = 0 if $cmd eq "add_dis";
	&trigger_list($trigstr) unless $Loading;
    } else {
	print "trigger: invalid command\n";
	&trigger_help();
    }
}
#*main::dirtcmd_trig = \&main::dirtcmd_trigger;

# Lists defined triggers (activated by '/trigger list')
sub trigger_list {
    my $t = shift;
    my $i = 0;
    my $ts;
    $t = "" unless defined $t;
    foreach (keys %Triggers) {
	if (++$i == 1 && $t eq "") {
	    print "Currently defined triggers:\n";
	}
	next unless $t eq "" || $_ eq $t;
	print "*" unless $Triggers{$_}->[1];
	($ts = $_) =~ s/"/\\"/g;
	$ts = "\"$_\"";
	printf "%2d) $ts $Triggers{$_}->[0]\n", $i;
    }
    print "No triggers defined.\n" if $i == 0;
}

# Help for triggers (activated by '/trigger help')
sub trigger_help {
    print <<EOF;
Usage: trigger [<command> [<arguments>]]

Valid commands are:
add <regexp> <action>	If the <regexp> is matched in an input line, <action>
			string is sent to the MUD.  If <regexp> must contain
			blanks, enclose it in double quotes ("").  Double quote
			itself must be preceded by a backslash if it is to be
			included in a quoted string.  <action> should not be
			quoted.

list			Display currently defined triggers.  Each trigger is
			preceded by its number, which is used to reference it
			in other commands.  Disabled triggers are printed with
			an asterisk (*) before the number.  An empty command
			also lists the triggers.

action <number> <newac>	Redefine the action of trigger <number> to <newac>.

delete <number>		Delete trigger <number>.

disable <number>	Disable trigger <number>.

enable <number>		Enable trigger <number>.
EOF
}

#&main::hook_add("output", "Trigger::trigscan", \&trigscan);
#&main::run($main::commandCharacter . "hook -a -p2147483645 -F -T OUTPUT -fL perl trigscan = Trigger::trigscan");

print "Loaded auto/trigger.pl\t(Cause commands to be executed when mud output is received)\n";

1;
