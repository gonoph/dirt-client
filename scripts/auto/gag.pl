# Gags package
package Gag;

use vars qw(%Gags);  # Variables that will be exported in Gag:: namespace
use Getopt::Std;
my($pri) = -2147483646; # lowest possible priority -- let other hooks get it first.
if(!defined %Gags) {
    %Gags = ();
}
my($regexdelim) = qr/[\#\/\%\&!,=:]/;           # regex delimiters

&main::run("/hook -T INIT -F -fL perl definegags = Gag::definegags");
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
        $hookcmd .= " " ." -p $pri -fL perl " . $name . " = Gag::gagit";
        &main::run($hookcmd);
    }
    &main::run($main::commandCharacter . "hook -d definegags"); # delete myself from INIT list.
}
&main::save("Gag::Gags", \%Gags);

sub gagit {
    $_ = "";
}

&main::run("/hook -T COMMAND -fL perl -C gag gag = Gag::command_gag");

# command '/gag' (list/add/delete gags)
sub command_gag {
    my(%opts);
    my(%gaghash);
    my($name);
    @ARGV = (); # reset it.
    if(/${main::commandCharacter}gag(.*)/) { $_ = $1; }
    else { &main::report_err("This doesn't seem to be a /gag command!\n"); }
# Line noise counts as perl?  Why, yes.
    while(/\G\s+(?:(-[A-Za-z]+)?\s*\"(.*?[[^\\](?:\\\\)*|)\"|(-[A-Za-z]+)?\s*\'(.*?[^\\](?:\\\\)*|)\'|(=)|(-[A-Za-z]*t) *($regexdelim)(.*?[^\\](?:\\\\)*)?\7|([^ \t\n"']+))/g) {
        if(defined $1) { 
            push @ARGV, $1; 
            push @ARGV, &main::debackslashify($2);
        } elsif(defined $2) { 
            push @ARGV, &main::debackslashify($2);
        } elsif(defined $3) {
            push @ARGV, $3;
            push @ARGV, &main::debackslashify($4);
        } elsif(defined $4) {
            push @ARGV, &main::debackslashify($4);
        } elsif(defined $5) { # The rest is the thing to be executed 
            push @ARGV, $5;
            if(m/\G\s+(.*)$/g) { push @ARGV, $1; }
            last;
        } elsif(defined $6) {
            push @ARGV, $6;
            push @ARGV, (defined $8)?$8:"";     # NO debackslashify.
        } elsif(defined $9) {
            push @ARGV, $9;
        } else {
            &main::report_err($main::commandCharacter . "gag: Error parsing command into \@ARGV\n");
            last;
        }
    }
    if(defined pos && pos != length) {
        &main::report_err($main::commandCharacter . "gag: did not reach end of argument string. \n");
    }
    getopts('lDFafd:p:c:n:g:L:t:', \%opts);
    my($fallthrough) = (0);
    my($hookcmd) = "/hook -T OUTPUT -fL perl -p $pri ";

    if(defined $opts{l} && $opts{l}) {
        &main::report(sprintf("%-35s%6s%6s %s\n", "Name", "Shots", "Flags", "Groups"));
        foreach $name (sort keys %Gags) {
            my($gagref) = $Gags{$name};
            &main::report(sprintf("%-35s%6s%2s%2s%2s %s\n", $name, $gagref->{n}, 
                $gagref->{F}?"F":"", $gagref->{D}?"D":"", $gagref->{a}?"C":"", $gagref->{g}));
            &main::report("\tGagging: ", $gagref->{t});
        }
        return 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Gags{$opts{d}}) {
            &main::run($main::commandCharacter . "hook -d " . $opts{d});
            delete $Gags{$opts{d}};
        } else {
            &main::report_err($main::commandCharacter . "gag: cannot delete gag '$opts{d}' because it isn't defined.\n");
        }
        return 1;
    }
    if(defined $opts{a}) { $hookcmd .= "-a "; $gaghash{a} = 1; }
    else { $gaghash{a} = 0; }
    if(defined $opts{D}) { $hookcmd .= "-D "; $gaghash{D} = 1; }
    else { $gaghash{D} = ""; }
    if(defined $opts{n}) { $hookcmd .= "-n '$opts{n}' "; $gaghash{n} = $opts{n}; }
    else { $gaghash{n} = -1; } # default shots = infinite
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
    $hookcmd .= "'" . $name . "' = Gag::gagit";
#    &main::report("hook command is: $hookcmd");
    &main::run($hookcmd);
    $Gags{$name} = \%gaghash;  # main::save will save complex data structures for us!
    return 1;
}

print "Loaded auto/gag.pl\t(Routines to gag mud output)\n";

1;
