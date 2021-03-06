# Gags package
package Gag;

use vars qw(%Gags);  # Variables that will be exported in Gag:: namespace
use Getopt::Std;
my($pri) = -2147483646; # lowest possible priority -- let other hooks get it first.
if(!defined %Gags) {
    %Gags = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters

&main::run("/hook -T INIT -F definegags = /run -Lperl Gag::definegags");
sub definegags {
    foreach my $name (keys %Gags) {
        my($hookcmd) = "/hook -T OUTPUT";
        foreach my $opt (keys %{$Gags{$name}}) {
            if($Gags{$name}->{$opt} && length($opt) == 1) {
                if($opt =~ /[iFeDaf]/) { 
                    $hookcmd .= " -" . $opt;
                } else {
                    $hookcmd .= " -" . $opt . " '" . &main::backslashify($Gags{$name}->{$opt}, '\'') . "'";
                }
            }
        }
        $hookcmd .= " " ." -p $pri '__DIRT_GAG_" . $name . "' = /run -Lperl Gag::gagit";
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definegags"); # delete myself from INIT list.
}
&main::save("Gag::Gags", \%Gags);

sub gagit {
    $_ = "";
}

&main::run("/hook -T COMMAND -C gag gag = /run -Lperl Gag::command_gag");

# command '/gag' (list/add/delete gags)
sub command_gag {
    my(%opts);
    my(%gaghash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}gag(.*)/) { $_ = $1; }
    else { &main::report_err("This doesn't seem to be a /gag command!\n"); }
# Line noise counts as perl?  Why, yes.
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
            &main::report_err($main::commandCharacter . "gag: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "gag: did not reach end of argument string. \n");
    }
    getopts('ad:DFln:g:t:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T OUTPUT -p $pri ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-35s%6s%6s %s\n", "Name", "Shots", "Flags", "Groups"));
        foreach $name (sort keys %Gags) {
            my($gagref) = $Gags{$name};
            &main::report(sprintf("%-35s%6s%2s%2s%2s %s\n", $name, $gagref->{n}, 
                $gagref->{F}?"F":"", $gagref->{D}?"D":"", $gagref->{a}?"C":"", $gagref->{g}));
            &main::report(sprintf("\tGagging: %s\n", $gagref->{t}));
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Gags{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d '__DIRT_GAG_" . $opts{d} . "'");
            delete $Gags{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "gag: cannot delete gag '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{a}) { $hookcmd .= "-a "; $gaghash{a} = 1; }
    else { $gaghash{a} = 0; }
    if(defined $opts{n}) { $hookcmd .= "-n "; $gaghash{n} = 1; }
    else { $gaghash{n} = 0; }
    if(defined $opts{D}) { $hookcmd .= "-D "; $gaghash{D} = 1; }
    else { $gaghash{D} = ""; }
    if(defined $opts{F}) { $hookcmd .= "-F "; $aliashash{F} = 1; }
    else { $aliashash{F} = 0; }
    if(defined $opts{g}) { $hookcmd .= "-g '$opts{g}' "; $gaghash{g} = $opts{g}; }
    else { $gaghash{g} = ""; }
    if(defined $opts{t}) { $hookcmd .= "-t '" . &main::backslashify($opts{t},"'") . "' "; $gaghash{t} = $opts{t}; }
    else { $gaghash{t} = ""; }

    if($#ARGV < 0) { # /gag was dropped already.
        &main::report_err($main::commandCharacter . "gag: Not enough arguments.  See /help gag.");
        for($i=0;$i<$#ARGV;$i++) { &main::report_err("\t$i: $ARGV[$i]\n"); }
        return 1;
    }
    $name = $ARGV[0];
    $hookcmd .= "'__DIRT_GAG_" . $name . "' = /run -Lperl Gag::gagit";
    &main::run($hookcmd);
    $Gags{$name} = \%gaghash;  # main::save will save complex data structures for us!
    return 1;
}

# Intercept /enable and /disable to keep our %Gags hash accurate
&main::run("/hook -T COMMAND -C enable Gag::command_enable = /run -Lperl Gag::command_enable");
sub command_enable {
    @ARGV = (); # reset it.
    if(!/^${main::commandCharacter}enable/g) { 
        &main::report_err("This doesn't seem to be an /enable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-G/g);
    return if(/-g/); # The only option to /enable -- the C++ /enable will generate a 
                     # bunch of /enable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Gags{$2}) {
            $Gags{$2}->{'D'} = 0;
	    &main::run("/enable '__DIRT_GAG_$2'");
        }
        # else ignore it...it may be a hook, not a alias.
    }
    return 1;
}

&main::run("/hook -T COMMAND -C disable Gag::command_disable = /run -Lperl Gag::command_disable");
sub command_disable {
    @ARGV = (); # reset it.
    if(!/${main::commandCharacter}disable/g) { 
        &main::report_err("This doesn't seem to be a /disable command!\n"); 
        return 1;
    }
    return if(!/\G\s+-G/g);
    return if(/-g/); # The only option to /disable -- the C++ /disable will generate a 
                     # bunch of /disable commands in this case.
    while(/\G\s+("|'|)([ :\w]+)\1/g) {
        if(defined $Gags{$2}) {
            $Gags{$2}->{'D'} = 1;
	    &main::run("/disable '__DIRT_GAG_$2'");
        }
        # else ignore it...it may be a hook, not a alias.
    }
    return 1;
}

print "Loaded auto/gag.pl\t(Routines to gag mud output)\n";

1;

