# Tab completion module
package Completion;

use vars qw(%Completions @Volatile);
use Getopt::Std;
# The length of the last completion, e.g. dest<tab>roy will have $LastLen=3
# so another TAB after that means that we should go that long back
# to look for a word
$LastCompletionLen = 0;
$PrevIndex = 0;  # Which completion did we pick the last time?
$CompletionRecursing = 0;
%Completions = ();
&main::save("Completion::Completions", \%Completions);

# Autocompletion parameters.
@Volatile = ();
$AutocompleteMin = 5;
&main::save("Completion::AutocompleteMin", \$AutocompleteMin);
$AutocompleteSize = 200;
&main::save("Completion::AutocompleteSize", \$AutocompleteSize);
$AutocompleteSmashCase = 0;
&main::save("Completion::AutocompleteSmashCase", \$AutocompleteSmashCase);

# Looks through @Volatile (created from mud output) and offers completions
sub tab_complete {
    my ($word,$complete,$n);
    unless (defined $main::Key) { &main::report_err("\$main::Key is not defined!!\n"); }
    if ($main::Key eq $main::keyTab) {
        # Try tab completion
        ($word) = /(\w*).{$LastCompletionLen}$/;
        foreach $complete (keys %Completions, reverse @Volatile) {
            if ($complete =~ /^$word/i and $n++ == $PrevIndex) {
                # Automatically stick in a space here ?
                s/\w*.{$LastCompletionLen}$/$complete /;
                $LastCompletionLen = (1+length($complete)) - length($word);
                $main::Key = 0;
                $PrevIndex = $n;
                return 1;
            }
        }

        # End of list but we have completed something before? Try recursing
        # But don't recurse if we only have one possible item
        if ($LastCompletionLen and !$CompletionRecursing and $PrevIndex > 1) {
            $CompletionRecursing = 1;
            $PrevIndex = 0;
            &tab_complete();
            $CompletionRecursing = 0;
        } else {
            &main::dirt_bell();
            $main::Key = 0;
        }
        return 1;
    } elsif ($main::Key eq $main::keyBackspace and $LastCompletionLen) { # Backspace deletes completed word
        s/\w*.{$LastCompletionLen}\s*$//;
        $LastCompletionLen = 0;
        $PrevIndex = 0;
        $main::Key = 0;
        return 1;
    }
    else {
        $LastCompletionLen = 0;
        $PrevIndex = 0;
    }
    return 0;
}

# list/add/delete static completions
&main::run($main::commandCharacter . "hook -T COMMAND -C complete -fL perl __DIRT_COMPLETE_command = Completion::command_complete");
sub command_complete {
    my ($count);
    my (%opts);
#    my ($defaultvar) = $_; # save it
    @ARGV = split /\s+/, $_;            # getopts uses @ARGV, not @_
    shift(@ARGV); # drop first arg.
    getopts('lcCm:s:a:d:', \%opts);
#    print "$_ running...\$opts{l}: ", (defined $opts{'l'})?$opts{'l'}:"undefined", "\n";

    if(defined $opts{c} && $opts{c} && defined $opts{C} && $opts{C}) {
        &main::report_err("/complete: Options -c and -C are mutually exclusive!\n");
    } elsif(defined $opts{c} && $opts{c}) {
        $AutocompleteSmashCase = 1;
    } elsif(defined $opts{C} && $opts{C}) {
        $AutocompleteSmashCase = 0;
    }
    if(defined $opts{m} && $opts{m}) {
        $AutocompleteMin = $opts{m};
    }
    if(defined $opts{s} && $opts{s}) {
        @Volatile = ();
        $AutocompleteSize = $opts{s};
    }
    if(defined $opts{a} && $opts{a}) {
        $Completions{$opts{a}} = 1;
    }
    if(defined $opts{d} && $opts{d}) {
        if(defined $Completions{$opts{d}}) {
            delete $Completions{$opts{d}};
            &main::report($main::commandCharacter . "complete: Deleted completion: $Completions{$opts{d}}\n");
        } else {
            &main::report_err($main::commandCharacter . "complete: cannot delete completion $opts{d} because it is not defined.\n");
        }
    }
    if(defined $opts{'l'} && $opts{'l'}) {
        &main::report("Current autocompletion parameters:\n");
        &main::report("    Minimum word size: $AutocompleteMin\n");
        &main::report("    Maximum history size: $AutocompleteSize\n");
        &main::report("    Lowercase all input: ", $AutocompleteSmashCase ? "on" : "off", "\n");
        &main::report("    Volatile completions list: \n");
        my($line) = "\t";
        foreach my $var (@Volatile) {
            $line .= $var . ", ";
            if(length $line  > 70) { &main::report($line . "\n"); $line = "\t"; }
        }
        &main::report($line . "\n");
        $line = "\t";
        &main::report("    Static completions list: \n");
        foreach my $var (keys %Completions) {
            $line .= $var . ", ";
            if(length $line  > 70) { &main::report($line . "\n"); $line = ""; }
        }
        &main::report($line . "\n");
    }
#    $_ = $defaultvar;
    return 1;
}


# gather volatile completions from mud output.
sub volatile_completions {
    my (@w, %w, $w, @v);
    my ($i, $n, $l);

    $l = $_;
    $_ = lc $l if $AutocompleteSmashCase;
    @w = /(\w{$AutocompleteMin,})/g;
    foreach $w (@w) {
	$w{$w} = 1;
    }
    while ($#Volatile > $AutocompleteSize - $#w - 1) {
	shift @Volatile;
    }
    for ($n = 0, $i = 0; $i <= $#Volatile; $i++) {
	next if defined $w{$Volatile[$i]};
	$v[$n++] = $Volatile[$i];
    }
    %w = ();
    for ($i = 0; $i <= $#w; $i++) {
	next if defined $w{$w[$i]};
	$w{$w[$i]} = 1;
	$v[$n++] = $w[$i];
    }
    @Volatile = @v;
    $_ = $l;
    return 1;
}

&main::run($main::commandCharacter . "hook -T KEYPRESS -fL perl __DIRT_COMPLETE_tab = Completion::tab_complete");

&main::run($main::commandCharacter . "hook -F -T OUTPUT -fL perl __DIRT_COMPLETE_volatile = Completion::volatile_completions");

print "Loaded auto/complete.pl\t(will complete words when you press <tab>)\n";

1;
