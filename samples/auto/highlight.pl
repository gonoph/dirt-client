# Highlights package
package Highlight;
use vars qw(%Highlights);  # vars that will be exported in Highlight:: namepace
if(!defined %Highlights) {
    %Highlights = ();
}
&main::save("Highlight::Highlights", \%Highlights);

# We use the color save/restore codes here
sub highlighter {
    foreach $p (keys %Highlights) {
        my($col) = &main::str_to_color($Highlights{$p});
        s/($p)/${main::CSave}$col$1${main::CRestore}/g;
    }
}

# Command '/highlight' (list/add/delete highlights)
sub main::dirtcmd_highlight {
    my ($col,$Color,$What,$count);
    
    if (/^(\S+)\s+(.*)/) { # Display highlights
        $Color = $1; $What = $2;
        if (length($col = &main::str_to_color($Color))) {
            $Highlights{$What} = $Color;
            print "Highlighting: ${col}${What}${main::White}\n" unless $Loading;
        } else {
            print "'$Color' is not a valid color code.\n";
        }
    } elsif (/^(\d+)/) {
        # Remove a certain highlight
        $count = 0;
        foreach (keys %Highlights) {
            if (++$count == $1) {
                my($col) = &main::str_to_color($Highlights{$_});
                print "Deleted: ${col}$_${main::White}\n";
                delete $Highlights{$_};
                return;
            }
        }
        print "No such highlight (there are only $count!) - type highlight to see a list.\n";
        
    } else {
        print "Following highlights are active:\n";
        foreach (keys %Highlights) {
            my($col) = &main::str_to_color($Highlights{$_});
            printf "%2d) ${col}$_${main::White}\n", ++$count;
        }
        print "\nUse highlight <color> <string> to highlight.\n";
        print "Use highlight <number> to remove.\n";
    }
}
*main::dirtcmd_hilite = \&main::dirtcmd_highlight;

# Save highlights to file
#sub save_highlights {
#    local (*FH) = $_[0];
#    foreach (keys %Highlights) {
#        print FH "highlight ", &color_to_str($Highlights{$_}), " $_\n";
#    }
#}

#save_add(\&save_highlights);
#load_add("highlight", \&dirtcmd_highlight);
&main::run($main::commandCharacter . "hook -T OUTPUT -p -1000 -F -fL perl -a highlighter = Highlight::highlighter");
#&main::hook_add("output", "highlighter", \&highlighter, -1000, 1); # absurdly low priority -- run after *everything*

print "Loaded auto/highlight.pl(Routines to highlight mud output)\n";

1;
