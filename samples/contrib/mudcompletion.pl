# Tab completion module, like completion.pl, but asks the MUD for the completion
# Experimental. Don't use, it does't even work on AR yet :)

package mudcompletion;

# The length of the last completion, e.g. dest<tab>roy will have $LastLen=3
# so another TAB after that means that we should go that long back
# to look for a word
$LastCompletionLen = 0;
$PrevIndex = 0;  # Which completion did we pick the last time?
$CompletionRecursing = 0;
$WaitingForCompletion = 0;
%Completions = ();
$LastInputLine = "";

sub complete_one {
    # Try tab completion
    ($word) = /(\s*).{$LastCompletionLen}$/;
    foreach $complete (keys %Completions) {
        if ($n++ == $PrevIndex) {
            # Automatically stick in a space here ?
            s/\s*.{$LastCompletionLen}$/$complete /;
            $LastCompletionLen = (1+length($complete)) - length($word);
            $main::Key = 0;
            $PrevIndex = $n;
            return;
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
    
}
sub tab_complete {
    my ($word,$complete,$n);
    if ($main::Key eq $main::keyTab) {
        # Are we cycling through a completion?
        if (scalar(keys %Completions) == 0) {
            ($word) = /(\w*).{$LastCompletionLen}\s*$/;
            if (!$WaitingForCompletion) {
                print "Asking for: $word\n";
                &main::dirt_send("tab_complete '$word' files world");
                $WaitingForCompletion = 1;
                $LastInputLine = $_;
            } else {
                &main::dirt_status("Still waiting for completion list. Server may not support tab completion or is slow.");
            }
            $main::Key = 0;
        } else { # Cycle through the received data
            &complete_one;
        }
    } elsif ($main::Key eq $main::keyBackspace and $LastCompletionLen) { # Backspace deletes completed word
        s/\s*.{$LastCompletionLen}\s*$//;
        $LastCompletionLen = 0;
        $PrevIndex = 0;
        $main::Key = 0;
        undef %Completions;
    }  else {
        $LastCompletionLen = 0;
        $PrevIndex = 0;
        undef %Completions;
    }
}

# TAB_COMPLETE:(.*)
sub check_completion {
    if (/^TAB_COMPLETE:(.*)/) {
        print "Completions have arrived: ", $1, "\n";
        undef %Completions;
        foreach (split(" ", $1)) {
            $Completions{$_} = 1;;
        }
        $WaitingForCompletion = 0;


        # Oops.
        $_ = $LastInputLine;
        &complete_one;
        &main::dirt_setinput($_);
        
        $_ = "";
    }
}

main::keypress_add(\&tab_complete);
main::output_add(\&check_completion);

package main;
1;
