# Alias Package

package Alias;
use vars qw(%Aliases); # vars that will be exported in the Alias:: namespace
use Getopt::Std;
if(!defined %Aliases) {
    %Aliases = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters
my($aliasargs)  = "(?:\\\\s+([^\\\\s]+))?" x 20;# set the regex that automatically matches alias arguments
#   $aliasargs   = $aliasargs;
    
&main::run("/hook -T INIT -F -fL perl definealiases = Alias::definealiases");
sub definealiases {
    foreach my $name (keys %Aliases) {
        my($hookcmd) = "/hook -T SEND";
        foreach my $opt (keys %{$Aliases{$name}}) {
            if($Aliases{$name}->{$opt} && length($opt) == 1) {
                if($opt =~ /[iFeDaf]/) { 
                    $hookcmd .= " -" . $opt;
                } else {
                    $hookcmd .= " -" . $opt . " '" . &main::backslashify($Aliases{$name}->{$opt}, '\'') . "'";
                }
            }
        }
        $hookcmd .= " -t'^$name$aliasargs\$' '__DIRT_ALIAS_" . $name . "' = " . $Aliases{$name}->{'action'};
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definealiases"); # delete myself from INIT list.
}
&main::save("Alias::Aliases", \%Aliases);

&main::run("/hook -T COMMAND -fL perl -C alias alias = Alias::command_alias");
sub command_alias {
    my(%opts);
    my(%aliashash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}alias(.*)/) { $_ = $1; }
    else { report_err("This doesn't seem to be a /alias command!\n"); }
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
            &main::report_err($main::commandCharacter . "alias: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "alias: did not reach end of argument string. \n");
    }
    getopts('d:DfFg:lL:p:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T SEND ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-15s%-35s%11s%6s %s\n", "Name", "Expansion", "Priority", "Flags", "Groups"));
        foreach $name (sort keys %Aliases) {
            my($aliasref) = $Aliases{$name};
            &main::report(sprintf("%-15s%-35s%11s%4s%2s %s\n", $name, $aliasref->{'action'}, 
                $aliasref->{p}, $aliasref->{F}?"F":"", $aliasref->{D}?"D":"", $aliasref->{g}));
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Aliases{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d '__DIRT_ALIAS_" . $opts{d} . "'");
            delete $Aliases{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "alias: cannot delete alias '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{D}) { $hookcmd .= "-D "; $aliashash{D} = 1; }
    else { $aliashash{D} = ""; }
    if(defined $opts{f}) { $hookcmd .= "-f "; $aliashash{f} = 1; }
    else { $aliashash{f} = 0; }
    if(defined $opts{F}) { $hookcmd .= "-F "; $aliashash{F} = 1; }
    else { $aliashash{F} = 0; }
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $aliashash{g} = $opts{g}; }
    else { $aliashash{g} = ""; }
    if(defined $opts{L}) { $hookcmd .= "-L '$opts{L}' "; $aliashash{L} = $opts{L}; }
    else { $aliashash{L} = ""; }
    if(defined $opts{p}) { $hookcmd .= "-p '$opts{p}' "; $aliashash{p} = $opts{p}; }
    else { $aliashash{p} = 0; } # default priority

    if($#ARGV < 2) { # /alias was dropped already.
        &main::report_err($main::commandCharacter . "alias: Not enough arguments.  See /help alias.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
    $aliashash{'action'} = join(" ", @ARGV[2..$#ARGV]);
    $hookcmd .= "-t'^$name$aliasargs\$' '__DIRT_ALIAS_" . $name . "' = " . $aliashash{'action'};
    &main::run($hookcmd);
    $Aliases{$name} = \%aliashash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Aliases hash accurate
&main::run("/hook -T COMMAND -fL perl -C enable Alias::command_enable = Alias::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-A/g);
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Aliases{$2}) {
            $Aliases{$2}->{'D'} = 0;
	    &main::run("/enable '__DIRT_ALIAS_$2'");
        }
        # else ignore it...it may be a hook, not a alias.
    }
    return 1;
}

&main::run("/hook -T COMMAND -fL perl -C disable Alias::command_disable = Alias::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-A/g);
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Aliases{$2}) {
            $Aliases{$2}->{'D'} = 1;
	    &main::run("/disable '__DIRT_ALIAS_$2'");
        }
        # else ignore it...it may be a hook, not a alias.
    }
    return 1;
}

print "Loaded auto/alias.pl\t(Shortcuts for commands)\n";

1;
