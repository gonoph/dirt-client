# Command Package

package Command;
use vars qw(%Commands); # vars that will be exported in the Command:: namespace
use Getopt::Std;
if(!defined %Commands) {
    %Commands = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters
    
&main::run("/hook -T INIT -F definecommands = /run -Lperl Command::definecommands");
sub definecommands {
    foreach my $name (keys %Commands) {
        my($hookcmd) = "/hook -T COMMAND";
        foreach my $opt (keys %{$Commands{$name}}) {
            if($Commands{$name}->{$opt} && length($opt) == 1) {
                if($opt =~ /[iFeDaf]/) { 
                    $hookcmd .= " -" . $opt;
                } else {
                    $hookcmd .= " -" . $opt . " '" . &main::backslashify($Commands{$name}->{$opt}, '\'') . "'";
                }
            }
        }
        $hookcmd .= " -C '$name' '__DIRT_USERCOMMAND_" . $name . "' = " . $Commands{$name}->{'action'};
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definecommands"); # delete myself from INIT list.
}
&main::save("Command::Commands", \%Commands);

&main::run("/hook -T COMMAND -C command command = /run -Lperl Command::command_command");
&main::run("/hook -T COMMAND -C com com = /run -Lperl Command::command_command"); # abbreviation
sub command_command {
    my(%opts);
    my(%commandhash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}com(?:mand)?(.*)/) { $_ = $1; }
    else { report_err("This doesn't seem to be a /command command!\n"); }
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
            &main::report_err($main::commandCharacter . "command: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "command: did not reach end of argument string. \n");
    }
    getopts('d:DFg:lp:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T COMMAND ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-15s%-35s%11s%6s %s\n", "Name", "Action", "Priority", "Flags", "Groups"));
        foreach $name (sort keys %Commands) {
            my($commandref) = $Commands{$name};
            &main::report(sprintf("%-15s%-35s%11s%2s%2s %s\n", $name, $commandref->{'action'}, $commandref->{p},
	    $commandref->{F}?"F":"", $commandref->{D}?"D":"", $commandref->{g}));
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Commands{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d '__DIRT_USERCOMMAND_" . $opts{d} . "'");
            delete $Commands{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "command: cannot delete command '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{D}) { $hookcmd .= "-D "; $commandhash{D} = 1; }
    else { $commandhash{D} = ""; }
    if(defined $opts{F}) { $hookcmd .= "-F "; $commandhash{F} = 1; }
    else { $commandhash{F} = 0; }
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $commandhash{g} = $opts{g}; }
    else { $commandhash{g} = ""; }
    if(defined $opts{p}) { $hookcmd .= "-p '$opts{p}' "; $commandhash{p} = $opts{p}; }
    else { $commandhash{p} = 0; } # default priority

    if($#ARGV < 2) { # /command was dropped already.
        &main::report_err($main::commandCharacter . "command: Not enough arguments.  See /help command.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
    $commandhash{'action'} = join(" ", @ARGV[2..$#ARGV]);
    $hookcmd .= "-C '$name' '__DIRT_USERCOMMAND_" . $name . "' = " . $commandhash{'action'};
    &main::run($hookcmd);
    $Commands{$name} = \%commandhash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Commands hash accurate
&main::run("/hook -T COMMAND -C enable Command::command_enable = /run -Lperl Command::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-C/g);
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Commands{$2}) {
            $Commands{$2}->{'D'} = 0;
	    &main::run("/enable '__DIRT_USERCOMMAND_$2'");
        }
        # else ignore it...it may be a hook, not a command.
    }
    return 1;
}

&main::run("/hook -T COMMAND -C disable Command::command_disable = /run -Lperl Command::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-C/g);
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Commands{$2}) {
            $Commands{$2}->{'D'} = 1;
	    &main::run("/disable '__DIRT_USERCOMMAND_$2'");
        }
        # else ignore it...it may be a hook, not a command.
    }
    return 1;
}

print "Loaded auto/command.pl\t(User defined commands)\n";

1;

